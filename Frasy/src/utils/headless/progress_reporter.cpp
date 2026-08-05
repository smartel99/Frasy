/**
 * @file    progress_reporter.cpp
 * @author  Frasy
 * @date    2026-08-05
 * @brief   Progress reporter implementation.
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
#include "progress_reporter.h"

#include <chrono>
#include <format>
#include <iostream>
#include <json.hpp>

namespace Frasy::Headless {

ProgressReporter::ProgressReporter(const std::string&              outputFormat,
                                   const std::string&              product,
                                   const std::vector<std::string>& serials,
                                   std::mutex&                     ioMutex)
: m_outputFormat(outputFormat), m_product(product), m_serials(serials), m_ioMutex(ioMutex)
{
}

void ProgressReporter::reportStart()
{
    std::lock_guard lock(m_ioMutex);
    if (m_outputFormat == "json") {
        nlohmann::json j;
        j["type"]      = "run_start";
        j["product"]   = m_product;
        j["serials"]   = m_serials;
        j["uuts"]      = m_serials.size();
        j["timestamp"] = timestamp();
        std::cout << j.dump() << "\n" << std::flush;
    }
    else {
        std::cout << std::format("[{}] Starting: product=\"{}\" uuts={}\n",
                                 timestamp(), m_product, m_serials.size())
                  << std::flush;
    }
}

void ProgressReporter::onEvent(const ProgressEvent& event)
{
    std::string serial = (event.uut >= 1 && event.uut <= m_serials.size()) ? m_serials[event.uut - 1] : "?";

    std::lock_guard lock(m_ioMutex);

    if (m_outputFormat == "json") {
        nlohmann::json j;
        j["uut"]       = event.uut;
        j["serial"]    = serial;
        j["timestamp"] = timestamp();

        switch (event.type) {
            case ProgressEvent::SequenceStart:
                j["type"]     = "sequence_start";
                j["sequence"] = event.name;
                break;
            case ProgressEvent::SequenceEnd:
                j["type"]     = "sequence_end";
                j["sequence"] = event.name;
                j["pass"]     = event.pass;
                break;
            case ProgressEvent::TestStart:
                j["type"]     = "test_start";
                j["sequence"] = event.sequence;
                j["test"]     = event.name;
                break;
            case ProgressEvent::TestEnd:
                j["type"]     = "test_end";
                j["sequence"] = event.sequence;
                j["test"]     = event.name;
                j["pass"]     = event.pass;
                break;
            case ProgressEvent::Expectation:
                j["type"]     = "expectation";
                j["sequence"] = event.sequence;
                j["test"]     = event.test;
                j["name"]     = event.name;
                j["pass"]     = event.pass;
                break;
        }
        std::cout << j.dump() << "\n" << std::flush;
    }
    else {
        switch (event.type) {
            case ProgressEvent::SequenceStart:
                std::cout << std::format("[{}] [UUT{}] >> {} \n",
                                         timestamp(), event.uut, event.name)
                          << std::flush;
                break;
            case ProgressEvent::SequenceEnd:
                std::cout << std::format("[{}] [UUT{}] {} << {}\n",
                                         timestamp(), event.uut,
                                         event.pass ? "[PASS]" : "[FAIL]", event.name)
                          << std::flush;
                break;
            case ProgressEvent::TestStart:
                std::cout << std::format("[{}] [UUT{}]   > {} > {}\n",
                                         timestamp(), event.uut, event.sequence, event.name)
                          << std::flush;
                break;
            case ProgressEvent::TestEnd:
                std::cout << std::format("[{}] [UUT{}]   {} {} > {}\n",
                                         timestamp(), event.uut,
                                         event.pass ? "[PASS]" : "[FAIL]",
                                         event.sequence, event.name)
                          << std::flush;
                break;
            case ProgressEvent::Expectation:
                std::cout << std::format("[{}] [UUT{}]     {} {}\n",
                                         timestamp(), event.uut,
                                         event.pass ? "[PASS]" : "[FAIL]",
                                         event.name)
                          << std::flush;
                break;
        }
    }
}

std::string ProgressReporter::timestamp()
{
    auto        now  = std::chrono::system_clock::now();
    auto        time = std::chrono::system_clock::to_time_t(now);
    auto        ms   = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm     tm   = {};
    localtime_s(&tm, &time);
    return std::format("{:02}:{:02}:{:02}.{:03}", tm.tm_hour, tm.tm_min, tm.tm_sec, ms.count());
}

}    // namespace Frasy::Headless
