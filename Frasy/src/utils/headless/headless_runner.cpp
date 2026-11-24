/**
 * @file    headless_runner.cpp
 * @author  Frasy
 * @date    2026-08-05
 * @brief   Headless test runner implementation.
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
#include "headless_runner.h"
#include "console_popup_handler.h"
#include "progress_reporter.h"

#include <Brigerad/Core/Log.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <json.hpp>
#include <thread>

namespace Frasy::Headless {

namespace {
constexpr auto s_tag = "Headless";
}

HeadlessRunner::HeadlessRunner(const CliArgs& args, ProductProvider& provider) : m_args(args), m_provider(provider)
{
}

int HeadlessRunner::run()
{
    using namespace std::chrono_literals;

    // 1. Discover products
    auto products = discoverProducts();
    if (products.empty()) {
        BR_LOG_ERROR(s_tag, "No products found in lua/user/");
        return 2;
    }

    // 2. Find the requested product
    auto it = std::ranges::find_if(products, [&](const ProductInfo& p) { return p.name == m_args.product; });
    if (it == products.end()) {
        BR_LOG_ERROR(s_tag, "Product '{}' not found. Available products:", m_args.product);
        for (const auto& p : products) {
            BR_LOG_ERROR(s_tag, "  - {}", p.name);
        }
        return 2;
    }
    const auto& product = *it;

    // 3. Validate serial numbers
    if (!validateSerials()) { return 2; }

    m_deviceViewer = std::make_unique<DeviceViewer>(m_canOpen);
    m_deviceViewer->onAttach();

    // 4. Set up orchestrator via ProductProvider
    BR_LOG_INFO(s_tag, "Setting up product '{}'...", product.name);
    if (!m_provider.setup(m_orchestrator, m_canOpen, product.name, product.environmentPath, product.testPath)) {
        BR_LOG_ERROR(s_tag, "ProductProvider::setup() failed for product '{}'", product.name);
        return 2;
    }

    // 5. Validate serial count matches UUT count
    const auto& map      = m_orchestrator.getMap();
    size_t      uutCount = map.uuts.size();
    if (m_args.serials.size() != uutCount) {
        BR_LOG_ERROR(s_tag,
                     "Serial count mismatch: provided {} serials but product '{}' has {} UUTs",
                     m_args.serials.size(),
                     product.name,
                     uutCount);
        return 2;
    }

    // 6. Build the serials vector (index 0 is a copy of index 1, matching existing convention)
    std::vector<std::string> serials;
    serials.reserve(m_args.serials.size() + 1);
    serials.push_back(m_args.serials.front());    // UUT0 = copy of UUT1
    for (const auto& sn : m_args.serials) {
        serials.push_back(sn);
    }

    // 7. Install console popup handler
    m_orchestrator.setPopupImport([this](sol::state_view lua, std::size_t uut, Lua::Orchestrator::Stage) {
        importHeadlessPopup(lua, uut, m_args.outputFormat, m_args.popupTimeoutSeconds, m_ioMutex);
    });

    // 8. Start progress reporter
    ProgressReporter progressReporter(m_args.outputFormat, product.name, m_args.serials, m_ioMutex);
    progressReporter.reportStart();

    m_orchestrator.setProgressCallback([&progressReporter](const std::string& type,
                                                           std::size_t        uut,
                                                           const std::string& name,
                                                           const std::string& parentA,
                                                           const std::string& parentB,
                                                           bool               pass) {
        ProgressEvent event;
        event.uut  = uut;
        event.name = name;
        event.pass = pass;
        if (type == "sequence_start") { event.type = ProgressEvent::SequenceStart; }
        else if (type == "sequence_end") {
            event.type = ProgressEvent::SequenceEnd;
        }
        else if (type == "test_start") {
            event.type     = ProgressEvent::TestStart;
            event.sequence = parentA;
        }
        else if (type == "test_end") {
            event.type     = ProgressEvent::TestEnd;
            event.sequence = parentA;
        }
        else if (type == "expectation") {
            event.type     = ProgressEvent::Expectation;
            event.sequence = parentA;
            event.test     = parentB;
        }
        else {
            return;
        }
        progressReporter.onEvent(event);
    });

    // 9. Run the solution
    BR_LOG_INFO(
      s_tag, "Running tests: product='{}' operator='{}' uuts={}", product.name, m_args.operatorName, uutCount);

    std::atomic<bool> done = false;
    m_orchestrator.runSolution(m_args.operatorName,
                               serials,
                               true,    // regenerate
                               m_args.skipVerification,
                               [&done] { done = true; });

    // 10. Wait for completion (poll)
    while (!done) {
        std::this_thread::sleep_for(100ms);
        // TODO: Task 6 will add progress reporting here
    }

    // 11. Post-test hook
    m_provider.onTestComplete(m_orchestrator);

    // 11. Report results
    return reportResults();
}

std::vector<ProductInfo> HeadlessRunner::discoverProducts()
{
    // Check if the provider defines an explicit product list
    auto providerProducts = m_provider.listProducts();
    if (!providerProducts.empty()) { return providerProducts; }

    // Fallback: filesystem discovery
    namespace fs = std::filesystem;
    std::vector<ProductInfo> products;

    try {
        for (const auto& entry : fs::recursive_directory_iterator("lua/user")) {
            if (!entry.is_directory()) { continue; }
            auto environment = fs::directory_entry(entry.path() / "environment.lua");
            if (!environment.exists()) { continue; }

            auto envPath = environment.path();
            envPath.replace_extension();    // Remove .lua extension
            std::string productName = entry.path().filename().string();

            products.emplace_back(ProductInfo {
              .name            = productName,
              .environmentPath = envPath.string(),
              .testPath        = entry.path().string(),
            });
        }
    }
    catch (const fs::filesystem_error& e) {
        BR_LOG_ERROR(s_tag, "Error scanning for products: {}", e.what());
    }

    return products;
}

bool HeadlessRunner::validateSerials()
{
    bool allValid = true;
    for (size_t i = 0; i < m_args.serials.size(); ++i) {
        if (!m_provider.validateSerialNumber(m_args.serials[i])) {
            BR_LOG_ERROR(s_tag, "Invalid serial number for UUT {}: '{}'", i + 1, m_args.serials[i]);
            allValid = false;
        }
    }
    return allValid;
}

std::string_view getStateLabel(UutState state) noexcept
{
    switch (state) {
        case UutState::Passed: return "PASS";
        case UutState::Failed: return "FAIL";
        case UutState::Error: return "ERROR";
        case UutState::IncompletePassed: return "INCOMPLETE_PASS";
        case UutState::IncompleteFailed: return "INCOMPLETE_FAIL";
        case UutState::Disabled:
        case UutState::Idle:
        case UutState::Waiting:
        case UutState::Running: break;
    }
    return "UNKNOWN";
}

std::string_view getVerdictLabel(RunVerdict verdict) noexcept
{
    switch (verdict) {
        case RunVerdict::Passed: return "PASS";
        case RunVerdict::Incomplete: return "INCOMPLETE";
        case RunVerdict::Failed: return "FAIL";
        case RunVerdict::Error: break;
    }
    return "ERROR";
}

int getExitCode(RunVerdict verdict) noexcept
{
    switch (verdict) {
        case RunVerdict::Passed: return 0;
        case RunVerdict::Failed: return 1;
        case RunVerdict::Incomplete: return 3;
        case RunVerdict::Error: break;
    }
    return 2;
}

int HeadlessRunner::reportResults()
{
    const auto& map     = m_orchestrator.getMap();
    auto        overall = RunVerdict::Passed;

    struct UutResult {
        std::size_t uut;
        std::string serial;
        UutState    state       = UutState::Error;
        RunVerdict  verdict     = RunVerdict::Error;
        int         testsPassed = 0;
        int         testsTotal  = 0;
        double      duration    = 0.0;
        std::string reportPath;
    };

    std::vector<UutResult> results;

    for (const auto& uut : map.uuts) {
        UutResult r;
        r.uut    = uut;
        r.serial = (uut >= 1 && uut <= m_args.serials.size()) ? m_args.serials[uut - 1] : "?";

        const auto uutState = m_orchestrator.getUutState(uut);
        r.state             = uutState;
        // A UUT without a result after a run is an error.
        r.verdict = getVerdict(uutState).value_or(RunVerdict::Error);
        overall   = std::max(overall, r.verdict);

        // Read the report JSON for detailed info
        r.reportPath = std::format("{}/last/{}.json", m_args.outputDir, uut);
        if (std::filesystem::exists(r.reportPath)) {
            try {
                std::ifstream ifs(r.reportPath);
                auto          report = nlohmann::json::parse(ifs);
                if (report.contains("info")) {
                    r.duration = report["info"].value("time", nlohmann::json::object()).value("elapsed", 0.0);
                }
                if (report.contains("sequences")) {
                    for (auto& [seqName, seq] : report["sequences"].items()) {
                        if (seq.contains("tests")) {
                            for (auto& [testName, test] : seq["tests"].items()) {
                                r.testsTotal++;
                                if (test.value("pass", false)) { r.testsPassed++; }
                            }
                        }
                    }
                }
            }
            catch (...) {
                // Report parsing failed — leave counts at 0
            }
        }
        else {
            if (uutState != UutState::Disabled) { overall = RunVerdict::Error; }
        }

        results.push_back(r);
    }

    // Print summary
    std::lock_guard lock(m_ioMutex);
    if (m_args.outputFormat == "json") {
        nlohmann::json summary;
        summary["type"]         = "run_end";
        summary["overall_pass"] = overall == RunVerdict::Passed;
        summary["product"]      = m_args.product;

        nlohmann::json uutArray = nlohmann::json::array();
        for (const auto& r : results) {
            nlohmann::json u;
            u["uut"]          = r.uut;
            u["serial"]       = r.serial;
            u["pass"]         = r.verdict == RunVerdict::Passed;
            u["state"]        = getStateLabel(r.state);
            u["tests_passed"] = r.testsPassed;
            u["tests_total"]  = r.testsTotal;
            u["duration"]     = r.duration;
            u["report"]       = r.reportPath;
            uutArray.push_back(u);
        }
        summary["uuts"] = uutArray;
        std::cout << summary.dump() << "\n" << std::flush;
    }
    else {
        constexpr auto colorReset = "\033[0m";
        constexpr auto colorBold  = "\033[1m";
        const auto     colorOf    = [](RunVerdict verdict) {
            switch (verdict) {
                case RunVerdict::Passed: return "\033[32m";
                case RunVerdict::Incomplete: return "\033[33m";
                case RunVerdict::Failed:
                case RunVerdict::Error: break;
            }
            return "\033[31m";
        };

        std::cout << "\n";
        std::cout << std::format("{:=<50}\n", "");
        std::cout << std::format("{} Test Results: {}{}\n", colorBold, m_args.product, colorReset);
        std::cout << std::format("{:=<50}\n", "");

        for (const auto& r : results) {
            std::cout << std::format(" {}[{}] UUT{} ({}): {}    [{}/{} tests, {:.2f}s]{}\n",
                                     colorOf(r.verdict),
                                     getVerdictLabel(r.verdict),
                                     r.uut,
                                     r.serial,
                                     getStateLabel(r.state),
                                     r.testsPassed,
                                     r.testsTotal,
                                     r.duration,
                                     colorReset);
        }

        std::cout << std::format("{:-<50}\n", "");
        std::cout << std::format(" {}Overall: {}{}\n", colorOf(overall), getVerdictLabel(overall), colorReset);
        std::cout << " Reports:";
        for (const auto& r : results) {
            std::cout << " " << r.reportPath;
        }
        std::cout << "\n";
        std::cout << std::format("{:=<50}\n", "");
        std::cout << std::flush;
    }

    return getExitCode(overall);
}

}    // namespace Frasy::Headless
