/**
 * @file    ping.cpp
 * @author  Paul Thomas
 * @date    2026-11-19
 * @brief   Checks whether a CANopen node answers to requests.
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this
 * program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "ping.h"

#include "utils/communication/can_open/to_string.h"

#include <chrono>
#include <cstring>
#include <format>
#include <future>
#include <span>

namespace Frasy::CanOpenViewer {

namespace {
//! The device type is a bit field (device profile and additional information), it reads best in hexadecimal.
std::string formatDeviceType(std::span<const uint8_t> bytes)
{
    if (bytes.size() != sizeof(uint32_t)) { return std::format("{} bytes, expected 4", bytes.size()); }
    uint32_t value = 0;
    std::memcpy(&value, bytes.data(), sizeof(value));    // CANopen is little-endian, like the PC.
    return std::format("0x{:08X}", value);
}
}    // namespace

void Ping::start(CanOpen::Node& node)
{
    if (isPending()) { return; }

    auto* sdo = node.sdoInterface();
    if (sdo == nullptr) {
        m_status  = Status::Dead;
        m_message = "The node has no SDO interface.";
        return;
    }

    m_request = sdo->uploadData(s_index, s_subIndex, s_timeoutMs, s_retries, false, CanOpen::VarType::Unsigned32);
    m_status  = Status::Pending;
    m_message = "Waiting for an answer...";
}

void Ping::update()
{
    using namespace std::chrono_literals;
    if (!isPending() || !m_request.has_value()) { return; }
    if (m_request->future.wait_for(0s) != std::future_status::ready) { return; }

    const auto result = m_request->future.get();
    if (result.has_value()) {
        m_status  = Status::Alive;
        m_message = std::format("Device type: {}", formatDeviceType(*result));
    }
    else {
        const auto abortCode = m_request->abortCode();
        switch (abortCode) {
            // The request did not get any answer, or never left (e.g., the CANopen stack is stopped).
            case CO_SDO_AB_TIMEOUT:
            case CO_SDO_AB_NONE:
                m_status  = Status::Dead;
                m_message = "No response (timeout).";
                break;
            case CO_SDO_AB_GENERAL:
                m_status  = Status::Dead;
                m_message = "The request could not be sent, or was dropped. Is CANopen running?";
                break;
            default:
                // Whatever the reason, something answered.
                m_status  = Status::Fault;
                m_message = std::format("The node answered with an error: {}", abortCode);
                break;
        }
    }
    m_request.reset();
}

}    // namespace Frasy::CanOpenViewer
