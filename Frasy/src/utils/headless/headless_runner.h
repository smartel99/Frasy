/**
 * @file    headless_runner.h
 * @author  Frasy
 * @date    2026-08-05
 * @brief   Headless test runner that drives the orchestrator without a GUI.
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
#ifndef FRASY_UTILS_HEADLESS_RUNNER_H
#define FRASY_UTILS_HEADLESS_RUNNER_H

#include "product_provider.h"
#include "utils/cli/cli_args.h"
#include "utils/communication/can_open/can_open.h"
#include "utils/lua/orchestrator/orchestrator.h"

#include <string>
#include <vector>

namespace Frasy::Headless {

/**
 * @brief Drives the full headless test lifecycle.
 *
 * Discovers products, validates serials, sets up the orchestrator via ProductProvider,
 * runs the test solution, and reports results — all without a GUI.
 */
class HeadlessRunner {
public:
    HeadlessRunner(const CliArgs& args, ProductProvider& provider);

    /// Run the full headless test lifecycle. Returns exit code (0/1/2).
    int run();

private:
    std::vector<ProductInfo> discoverProducts();
    bool                     validateSerials();
    int                      reportResults();

    const CliArgs&    m_args;
    ProductProvider&  m_provider;
    Lua::Orchestrator m_orchestrator;
    CanOpen::CanOpen  m_canOpen;
    std::mutex        m_ioMutex;
};

}    // namespace Frasy::Headless

#endif    // FRASY_UTILS_HEADLESS_RUNNER_H
