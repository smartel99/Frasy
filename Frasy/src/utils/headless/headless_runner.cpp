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

HeadlessRunner::HeadlessRunner(const CliArgs& args, ProductProvider& provider)
: m_args(args), m_provider(provider)
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
    if (!validateSerials()) {
        return 2;
    }

    // 4. Set up orchestrator via ProductProvider
    BR_LOG_INFO(s_tag, "Setting up product '{}'...", product.name);
    if (!m_provider.setup(m_orchestrator, m_canOpen, product.name, product.environmentPath, product.testPath)) {
        BR_LOG_ERROR(s_tag, "ProductProvider::setup() failed for product '{}'", product.name);
        return 2;
    }

    // 5. Validate serial count matches UUT count
    const auto& map     = m_orchestrator.getMap();
    size_t      uutCount = map.uuts.size();
    if (m_args.serials.size() != uutCount) {
        BR_LOG_ERROR(s_tag, "Serial count mismatch: provided {} serials but product '{}' has {} UUTs",
                     m_args.serials.size(), product.name, uutCount);
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
    m_orchestrator.setPopupImport(
      [this](sol::state_view lua, std::size_t uut, Lua::Orchestrator::Stage stage) {
          importHeadlessPopup(lua, uut, m_args.outputFormat, m_args.popupTimeoutSeconds, m_ioMutex);
      });

    // 8. Start progress reporter
    ProgressReporter progressReporter(m_args.outputFormat, product.name, m_args.serials, m_ioMutex);
    progressReporter.reportStart();

    m_orchestrator.setProgressCallback(
      [&progressReporter](const std::string& type, std::size_t uut, const std::string& name,
                           const std::string& parentA, const std::string& parentB, bool pass) {
          ProgressEvent event;
          event.uut  = uut;
          event.name = name;
          event.pass = pass;
          if (type == "sequence_start") { event.type = ProgressEvent::SequenceStart; }
          else if (type == "sequence_end") { event.type = ProgressEvent::SequenceEnd; }
          else if (type == "test_start") { event.type = ProgressEvent::TestStart; event.sequence = parentA; }
          else if (type == "test_end") { event.type = ProgressEvent::TestEnd; event.sequence = parentA; }
          else if (type == "expectation") { event.type = ProgressEvent::Expectation; event.sequence = parentA; event.test = parentB; }
          else { return; }
          progressReporter.onEvent(event);
      });

    // 9. Run the solution
    BR_LOG_INFO(s_tag, "Running tests: product='{}' operator='{}' uuts={}",
                product.name, m_args.operatorName, uutCount);

    std::atomic<bool> done = false;
    m_orchestrator.runSolution(
      m_args.operatorName,
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

std::vector<HeadlessRunner::ProductInfo> HeadlessRunner::discoverProducts()
{
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

int HeadlessRunner::reportResults()
{
    const auto& map = m_orchestrator.getMap();
    bool        anyFailed = false;
    bool        anyError  = false;

    for (const auto& uut : map.uuts) {
        auto state = m_orchestrator.getUutState(uut);
        switch (state) {
            case UutState::Passed:
                BR_LOG_INFO(s_tag, "UUT {} ({}): PASSED", uut, m_args.serials[uut - 1]);
                break;
            case UutState::Failed:
                BR_LOG_ERROR(s_tag, "UUT {} ({}): FAILED", uut, m_args.serials[uut - 1]);
                anyFailed = true;
                break;
            case UutState::Error:
                BR_LOG_ERROR(s_tag, "UUT {} ({}): ERROR", uut, m_args.serials[uut - 1]);
                anyError = true;
                break;
            default:
                BR_LOG_WARN(s_tag, "UUT {} ({}): unexpected state {}", uut, m_args.serials[uut - 1],
                            static_cast<int>(state));
                anyError = true;
                break;
        }
    }

    // TODO: Task 7 will add full summary output (human/json format)

    if (anyError) { return 2; }
    if (anyFailed) { return 1; }
    return 0;
}

}    // namespace Frasy::Headless
