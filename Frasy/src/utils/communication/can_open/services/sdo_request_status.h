/**
 * @file    sdo_request_status.h
 * @author  Samuel Martel
 * @date    2024-05-09
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */


#ifndef FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_REQUEST_STATUS_H
#define FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_REQUEST_STATUS_H

#include <cstdint>

namespace Frasy::CanOpen {
enum class SdoRequestStatus : uint8_t {
    Unknown = 0,        //!< Status not known.
    Queued,             //!< Transfer hasn't started yet.
    OnGoing,            //!< Transfer is currently on going.
    Complete,           //!< Transfer has been completed.
    CancelRequested,    //!< Cancel requested, but not yet served.
    Cancelled,          //!< Transfer has been cancelled.
};
}

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_REQUEST_STATUS_H
