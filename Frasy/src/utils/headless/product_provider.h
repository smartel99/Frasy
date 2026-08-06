/**
 * @file    product_provider.h
 * @author  Frasy
 * @date    2026-08-05
 * @brief   Interface for application-specific product configuration in headless mode.
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
#ifndef FRASY_UTILS_HEADLESS_PRODUCT_PROVIDER_H
#define FRASY_UTILS_HEADLESS_PRODUCT_PROVIDER_H

#include <string>
#include <vector>

namespace Frasy {
namespace Lua {
class Orchestrator;
}
namespace CanOpen {
class CanOpen;
}

namespace Headless {

/**
 * @brief Describes a product available for testing.
 *
 * Used by ProductProvider::listProducts() to define explicit product-name-to-directory
 * mappings, allowing multiple product names to share test directories.
 * Also used internally by HeadlessRunner and McpRunner for product discovery.
 */
struct ProductInfo {
    std::string name;            ///< User-facing product name (e.g., "KR35626")
    std::string environmentPath; ///< Path to environment file without .lua extension
    std::string testPath;        ///< Path to the test directory
};

/**
 * @brief Interface that applications implement to provide product-specific orchestrator setup.
 *
 * This interface is shared between the GUI path and the headless runner. Applications register
 * an implementation via Frasy::Interpreter::setProductProvider(). Both the headless runner and
 * the GUI's makeOrchestrator can call setup() to configure the orchestrator for a given product.
 */
class ProductProvider {
public:
    virtual ~ProductProvider() = default;

    /**
     * @brief Provide an explicit list of available products.
     *
     * Override this to define product-name-to-directory mappings. When non-empty,
     * headless and MCP runners use this list instead of filesystem discovery.
     * This allows multiple product names to map to the same test directory.
     *
     * Default implementation returns empty (runners fall back to filesystem scan).
     *
     * @return Vector of product entries, or empty to use filesystem discovery.
     */
    virtual std::vector<ProductInfo> listProducts() { return {}; }

    /**
     * @brief Validate a serial number for the current product.
     * @param serial The serial number string to validate.
     * @return true if the serial number is valid, false otherwise.
     */
    virtual bool validateSerialNumber(const std::string& serial) = 0;

    /**
     * @brief Set up the orchestrator and CANopen for the given product.
     *
     * Responsibilities of the implementation:
     *   1. Select product-specific configuration based on the product name
     *   2. Call orchestrator.setLoadUserValues(...)
     *   3. Call orchestrator.loadUserFiles(envPath, testsDir)
     *   4. Configure CANopen nodes from the IB map
     *   5. Start CANopen
     *   6. Call orchestrator.setLoadUserFunctions(...)
     *
     * @param orchestrator The orchestrator instance to configure.
     * @param canOpen The CANopen instance to configure.
     * @param product The product name (as returned by listProducts or directory name).
     * @param envPath Path to the environment file (without .lua extension).
     * @param testsDir Path to the tests directory for this product.
     * @return true on success, false on failure.
     */
    virtual bool setup(Lua::Orchestrator& orchestrator,
                       CanOpen::CanOpen&   canOpen,
                       const std::string&  product,
                       const std::string&  envPath,
                       const std::string&  testsDir) = 0;

    /**
     * @brief Called after test execution completes.
     *
     * Optional hook for post-test actions (e.g., report sending, LED signaling).
     * Default implementation does nothing.
     *
     * @param orchestrator The orchestrator that just finished executing.
     */
    virtual void onTestComplete(Lua::Orchestrator& orchestrator) { (void)orchestrator; }
};

}    // namespace Headless
}    // namespace Frasy

#endif    // FRASY_UTILS_HEADLESS_PRODUCT_PROVIDER_H
