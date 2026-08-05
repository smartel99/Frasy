/**
 * @file    mcp_runner.cpp
 * @author  Frasy
 * @date    2026-08-05
 * @brief   MCP runner implementation.
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "mcp_runner.h"

#include <Brigerad/Core/Log.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace Frasy::Mcp {

namespace {
constexpr auto s_tag = "MCP-Runner";
}

McpRunner::McpRunner(Headless::ProductProvider& provider)
: m_provider(provider), m_server("frasy", "1.0.0")
{
}

int McpRunner::run()
{
    registerTools();
    m_server.run();

    // Wait for orchestrator to finish if tests are still running
    if (m_running) {
        BR_LOG_INFO(s_tag, "Waiting for running tests to complete before shutdown...");
        while (m_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // Clean shutdown
    m_canOpen.stop();
    return 0;
}

std::vector<McpRunner::ProductInfo> McpRunner::discoverProducts()
{
    namespace fs = std::filesystem;
    std::vector<ProductInfo> products;

    try {
        for (const auto& entry : fs::recursive_directory_iterator("lua/user")) {
            if (!entry.is_directory()) { continue; }
            auto environment = fs::directory_entry(entry.path() / "environment.lua");
            if (!environment.exists()) { continue; }

            auto envPath = environment.path();
            envPath.replace_extension();
            std::string productName = entry.path().filename().string();

            products.emplace_back(ProductInfo {
              .environmentPath = envPath.string(),
              .testPath        = entry.path().string(),
              .name            = productName,
            });
        }
    }
    catch (const fs::filesystem_error& e) {
        BR_LOG_ERROR(s_tag, "Error scanning for products: {}", e.what());
    }

    return products;
}

void McpRunner::registerTools()
{
    using json = nlohmann::json;

    // list_products
    m_server.registerTool(
      "list_products",
      "List available test products",
      json({{"type", "object"}, {"properties", json::object()}}),
      [this](const json& args) { return handleListProducts(args); });

    // run_tests
    m_server.registerTool(
      "run_tests",
      "Start a test run for a product. Returns immediately; poll get_status for progress.",
      json({{"type", "object"},
            {"properties",
             {{"product", {{"type", "string"}, {"description", "Product name to test"}}},
              {"operator", {{"type", "string"}, {"description", "Operator name"}}},
              {"serials", {{"type", "array"}, {"items", {{"type", "string"}}}, {"description", "Serial numbers, one per UUT"}}},
              {"skip_verification", {{"type", "boolean"}, {"description", "Skip hash verification (optional, default false)"}}}}},
            {"required", json::array({"product", "operator", "serials"})}}),
      [this](const json& args) { return handleRunTests(args); });

    // get_status
    m_server.registerTool(
      "get_status",
      "Get the current test execution status and per-UUT states.",
      json({{"type", "object"}, {"properties", json::object()}}),
      [this](const json& args) { return handleGetStatus(args); });

    // get_results
    m_server.registerTool(
      "get_results",
      "Get the test results summary after execution completes.",
      json({{"type", "object"}, {"properties", json::object()}}),
      [this](const json& args) { return handleGetResults(args); });

    // get_pending_popup
    m_server.registerTool(
      "get_pending_popup",
      "Get the next popup waiting for operator interaction. Returns null if none pending.",
      json({{"type", "object"}, {"properties", json::object()}}),
      [this](const json& args) { return handleGetPendingPopup(args); });

    // respond_to_popup
    m_server.registerTool(
      "respond_to_popup",
      "Respond to a pending popup by providing inputs and pressing a button.",
      json({{"type", "object"},
            {"properties",
             {{"id", {{"type", "string"}, {"description", "Popup ID from get_pending_popup"}}},
              {"inputs", {{"type", "object"}, {"description", "Input values keyed by index (e.g. {\"1\": \"value\"})"}}},
              {"button", {{"type", "string"}, {"description", "Button label to press"}}}}},
            {"required", json::array({"id", "button"})}}),
      [this](const json& args) { return handleRespondToPopup(args); });
}

nlohmann::json McpRunner::makeToolResult(const std::string& text, bool isError)
{
    nlohmann::json result;
    result["content"] = nlohmann::json::array({{{"type", "text"}, {"text", text}}});
    result["isError"] = isError;
    return result;
}

// --- Tool Handlers ---

nlohmann::json McpRunner::handleListProducts(const nlohmann::json& /*args*/)
{
    auto products = discoverProducts();

    nlohmann::json productList = nlohmann::json::array();
    for (const auto& p : products) {
        productList.push_back({{"name", p.name}});
    }

    nlohmann::json response;
    response["products"] = productList;
    return makeToolResult(response.dump());
}

nlohmann::json McpRunner::handleRunTests(const nlohmann::json& args)
{
    if (m_running) {
        return makeToolResult("{\"error\":\"Tests are already running\"}", true);
    }

    std::string product           = args.value("product", "");
    std::string operatorName      = args.value("operator", "");
    auto        serials           = args.value("serials", std::vector<std::string> {});
    bool        skipVerification  = args.value("skip_verification", false);

    if (product.empty() || operatorName.empty() || serials.empty()) {
        return makeToolResult("{\"error\":\"product, operator, and serials are required\"}", true);
    }

    // Validate serials
    for (const auto& sn : serials) {
        if (!m_provider.validateSerialNumber(sn)) {
            return makeToolResult(nlohmann::json({{"error", "Invalid serial number: " + sn}}).dump(), true);
        }
    }

    // Find product
    auto products = discoverProducts();
    auto it       = std::ranges::find_if(products, [&](const ProductInfo& p) { return p.name == product; });
    if (it == products.end()) {
        nlohmann::json available = nlohmann::json::array();
        for (const auto& p : products) { available.push_back(p.name); }
        return makeToolResult(
          nlohmann::json({{"error", "Product not found"}, {"available", available}}).dump(), true);
    }

    // Setup orchestrator
    if (!m_provider.setup(m_orchestrator, m_canOpen, it->name, it->environmentPath, it->testPath)) {
        return makeToolResult("{\"error\":\"ProductProvider::setup() failed\"}", true);
    }

    // Validate UUT count
    const auto& map = m_orchestrator.getMap();
    if (serials.size() != map.uuts.size()) {
        return makeToolResult(
          nlohmann::json({{"error", "Serial count mismatch"},
                          {"provided", serials.size()},
                          {"expected", map.uuts.size()}})
            .dump(),
          true);
    }

    // Install MCP popup handler
    m_orchestrator.setPopupImport(
      [this](sol::state_view lua, std::size_t uut, Lua::Orchestrator::Stage /*stage*/) {
          m_popupHandler.importPopup(lua, uut);
      });

    // Build serials vector (index 0 = copy of index 1)
    m_serials.clear();
    m_serials.reserve(serials.size() + 1);
    m_serials.push_back(serials.front());
    for (const auto& sn : serials) {
        m_serials.push_back(sn);
    }

    m_activeProduct = it->name;
    m_running       = true;

    // Launch async
    m_orchestrator.runSolution(
      operatorName,
      m_serials,
      true,    // regenerate
      skipVerification,
      [this] {
          m_running = false;
          m_provider.onTestComplete(m_orchestrator);
      });

    return makeToolResult("{\"started\":true}");
}

nlohmann::json McpRunner::handleGetStatus(const nlohmann::json& /*args*/)
{
    nlohmann::json status;

    if (m_running) {
        status["state"] = "running";
    }
    else if (m_activeProduct.empty()) {
        status["state"] = "idle";
    }
    else {
        // Determine overall state from UUT states
        const auto& map     = m_orchestrator.getMap();
        bool        anyFail = false;
        bool        anyErr  = false;
        for (const auto& uut : map.uuts) {
            auto s = m_orchestrator.getUutState(uut);
            if (s == UutState::Failed) anyFail = true;
            if (s == UutState::Error) anyErr = true;
        }
        if (anyErr) status["state"] = "error";
        else if (anyFail) status["state"] = "failed";
        else status["state"] = "passed";
    }

    // Per-UUT states
    const auto& map = m_orchestrator.getMap();
    nlohmann::json uuts = nlohmann::json::array();
    for (const auto& uut : map.uuts) {
        auto state = m_orchestrator.getUutState(uut);
        std::string stateStr;
        switch (state) {
            case UutState::Disabled: stateStr = "disabled"; break;
            case UutState::Idle: stateStr = "idle"; break;
            case UutState::Waiting: stateStr = "waiting"; break;
            case UutState::Running: stateStr = "running"; break;
            case UutState::Passed: stateStr = "passed"; break;
            case UutState::Failed: stateStr = "failed"; break;
            case UutState::Error: stateStr = "error"; break;
        }
        nlohmann::json uutInfo;
        uutInfo["uut"]    = uut;
        uutInfo["serial"] = (uut < m_serials.size()) ? m_serials[uut] : "";
        uutInfo["state"]  = stateStr;
        uuts.push_back(uutInfo);
    }
    status["uuts"]    = uuts;
    status["product"] = m_activeProduct;

    return makeToolResult(status.dump());
}

nlohmann::json McpRunner::handleGetResults(const nlohmann::json& /*args*/)
{
    if (m_running) {
        return makeToolResult("{\"error\":\"Tests are still running\"}", true);
    }
    if (m_activeProduct.empty()) {
        return makeToolResult("{\"error\":\"No test results available (no tests have been run)\"}", true);
    }

    const auto& map = m_orchestrator.getMap();
    nlohmann::json results;
    results["product"]      = m_activeProduct;
    results["overall_pass"] = true;

    nlohmann::json uutResults = nlohmann::json::array();
    for (const auto& uut : map.uuts) {
        auto state = m_orchestrator.getUutState(uut);
        bool pass  = (state == UutState::Passed);
        if (!pass) results["overall_pass"] = false;

        nlohmann::json uutResult;
        uutResult["uut"]    = uut;
        uutResult["serial"] = (uut < m_serials.size()) ? m_serials[uut] : "";
        uutResult["pass"]   = pass;

        std::string stateStr;
        switch (state) {
            case UutState::Passed: stateStr = "passed"; break;
            case UutState::Failed: stateStr = "failed"; break;
            case UutState::Error: stateStr = "error"; break;
            default: stateStr = "unknown"; break;
        }
        uutResult["state"] = stateStr;

        // Try to read the report file for detailed info
        std::string reportPath = std::format("logs/last/{}.json", uut);
        if (std::filesystem::exists(reportPath)) {
            uutResult["report_path"] = reportPath;
            try {
                std::ifstream ifs(reportPath);
                auto          report = nlohmann::json::parse(ifs);
                if (report.contains("info")) {
                    uutResult["duration"] = report["info"].value("time", nlohmann::json::object()).value("elapsed", 0.0);
                }
                // Count tests
                int totalTests  = 0;
                int passedTests = 0;
                if (report.contains("sequences")) {
                    for (auto& [seqName, seq] : report["sequences"].items()) {
                        if (seq.contains("tests")) {
                            for (auto& [testName, test] : seq["tests"].items()) {
                                totalTests++;
                                if (test.value("pass", false)) passedTests++;
                            }
                        }
                    }
                }
                uutResult["tests_passed"] = passedTests;
                uutResult["tests_total"]  = totalTests;
            }
            catch (...) {
                // Report parsing failed — just include the path
            }
        }

        uutResults.push_back(uutResult);
    }
    results["uuts"] = uutResults;

    return makeToolResult(results.dump());
}

nlohmann::json McpRunner::handleGetPendingPopup(const nlohmann::json& /*args*/)
{
    auto popup = m_popupHandler.getPendingPopup();
    if (popup.has_value()) {
        return makeToolResult(popup->dump());
    }
    return makeToolResult("{\"popup\":null}");
}

nlohmann::json McpRunner::handleRespondToPopup(const nlohmann::json& args)
{
    std::string id     = args.value("id", "");
    std::string button = args.value("button", "");

    if (id.empty() || button.empty()) {
        return makeToolResult("{\"error\":\"id and button are required\"}", true);
    }

    std::map<int, std::string> inputs;
    if (args.contains("inputs") && args["inputs"].is_object()) {
        for (auto& [key, value] : args["inputs"].items()) {
            try {
                inputs[std::stoi(key)] = value.get<std::string>();
            }
            catch (...) {
                // Skip invalid keys
            }
        }
    }

    if (m_popupHandler.respondToPopup(id, inputs, button)) {
        return makeToolResult("{\"ok\":true}");
    }
    return makeToolResult("{\"error\":\"Popup not found with id: " + id + "\"}", true);
}

}    // namespace Frasy::Mcp
