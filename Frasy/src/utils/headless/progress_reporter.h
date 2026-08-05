/**
 * @file    progress_reporter.h
 * @author  Frasy
 * @date    2026-08-05
 * @brief   Progress event types and reporter for headless mode.
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
#ifndef FRASY_UTILS_HEADLESS_PROGRESS_REPORTER_H
#define FRASY_UTILS_HEADLESS_PROGRESS_REPORTER_H

#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace Frasy::Headless {

/**
 * @brief Represents a progress event during test execution.
 */
struct ProgressEvent {
    enum Type {
        SequenceStart,
        SequenceEnd,
        TestStart,
        TestEnd,
        Expectation,
    };

    Type        type;
    std::size_t uut;
    std::string name;        ///< Sequence name, test name, or expectation name
    std::string sequence;    ///< Parent sequence (for test/expectation events)
    std::string test;        ///< Parent test (for expectation events)
    bool        pass = true; ///< Result (meaningful for End and Expectation events)
};

/**
 * @brief Receives progress events and outputs them to stdout.
 *
 * Installed as a callback on the orchestrator. Events are pushed directly
 * from the Lua execution thread — no polling needed.
 */
class ProgressReporter {
public:
    ProgressReporter(const std::string&              outputFormat,
                     const std::string&              product,
                     const std::vector<std::string>& serials,
                     std::mutex&                     ioMutex);

    /// Called when a progress event occurs (from any thread).
    void onEvent(const ProgressEvent& event);

    /// Print the run start header.
    void reportStart();

private:
    std::string timestamp();

    std::string              m_outputFormat;
    std::string              m_product;
    std::vector<std::string> m_serials;
    std::mutex&              m_ioMutex;
};

}    // namespace Frasy::Headless

#endif    // FRASY_UTILS_HEADLESS_PROGRESS_REPORTER_H
