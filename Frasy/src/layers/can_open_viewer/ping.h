/**
 * @file    ping.h
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
#ifndef FRASY_LAYERS_CAN_OPEN_VIEWER_PING_H
#define FRASY_LAYERS_CAN_OPEN_VIEWER_PING_H

#include "utils/communication/can_open/node.h"
#include "utils/communication/can_open/services/sdo_uploader.h"

#include <cstdint>
#include <optional>
#include <string>

namespace Frasy::CanOpenViewer {

/**
 * @brief Pings a node by reading its device type (0x1000).
 *
 * CANopen has no generic "ping" command. Object 0x1000 is mandatory in every device, so a SDO upload of it is a
 * simple way to tell that the node is alive and able to serve SDO requests, which is what the tests need.
 */
class Ping {
public:
    enum class Status : uint8_t {
        Idle,       //!< No ping has been made yet.
        Pending,    //!< Waiting for the answer.
        Alive,      //!< The node answered with the device type.
        Fault,      //!< The node answered, but with an error.
        Dead,       //!< The node did not answer, or the request could not be sent.
    };

    /**
     * @brief Sends the request. Does nothing if a ping is already in progress.
     */
    void start(CanOpen::Node& node);

    /**
     * @brief Collects the answer once it is available. Meant to be called regularly (e.g., every frame).
     */
    void update();

    [[nodiscard]] Status                    getStatus() const noexcept { return m_status; }
    [[nodiscard]] bool                      isPending() const noexcept { return m_status == Status::Pending; }
    //! Details about the outcome of the last ping, suitable for a tooltip.
    [[nodiscard]] const std::string&        getMessage() const noexcept { return m_message; }

private:
    //! Device type, mandatory in every CANopen device.
    static constexpr uint16_t s_index     = 0x1000;
    static constexpr uint8_t  s_subIndex  = 0;
    static constexpr uint16_t s_timeoutMs = 500;
    //! A node that does not answer is not going to start doing so on the second attempt, no retries.
    static constexpr uint8_t s_retries = 0;

    Status                                     m_status = Status::Idle;
    std::string                                m_message;
    std::optional<CanOpen::SdoUploadDataResult> m_request;
};

}    // namespace Frasy::CanOpenViewer

#endif    // FRASY_LAYERS_CAN_OPEN_VIEWER_PING_H
