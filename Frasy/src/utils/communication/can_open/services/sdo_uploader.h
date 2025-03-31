/**
 * @file    sdo_uploader.h
 * @author  Samuel Martel
 * @date    2024-05-07
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


#ifndef FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_UPLOADER_H
#define FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_UPLOADER_H

#include "sdo_request_status.h"
#include "var_type.h"

#include <CO_SDOclient.h>

#include <cstdint>
#include <expected>
#include <future>
#include <memory>
#include <span>
#include <vector>

namespace Frasy::CanOpen {

struct SdoUploadRequest {
    SdoRequestStatus status       = SdoRequestStatus::Unknown;
    uint8_t          nodeId       = 0;
    uint16_t         index        = 0;
    uint8_t          subIndex     = 0;
    bool             isBlock      = false;
    VarType          varType      = VarType::Undefined;
    uint16_t         sdoTimeoutMs = 1000;
    uint8_t          retries      = 5;    // number of time it will retry the transfer if needed.

    //! Total size of the data which will be transferred. It is optionally used by the server.
    size_t sizeIndicated = 0;
    //! Number of bytes that has been sent so far.
    size_t sizeTransferred = 0;

    CO_SDO_abortCode_t                                               abortCode = CO_SDO_AB_NONE;
    std::promise<std::expected<std::span<uint8_t>, CO_SDO_return_t>> promise;

    std::vector<uint8_t> data;

    void markAsComplete(const std::expected<std::span<uint8_t>, CO_SDO_return_t>& res)
    {
        if (status == SdoRequestStatus::CancelRequested) {
            cancel();
            return;
        }
        status = SdoRequestStatus::Complete;
        promise.set_value(res);
    }

    void cancel() { abort(CO_SDO_AB_GENERAL); }

    void abort(CO_SDO_abortCode_t code)
    {
        abortCode = code;
        status    = SdoRequestStatus::Cancelled;
        promise.set_value(std::unexpected {CO_SDO_RT_endedWithClientAbort});
    }
};

struct SdoUploadDataResult {
    [[nodiscard]] SdoRequestStatus   status() const { return m_request->status; }
    [[nodiscard]] uint8_t            nodeId() const { return m_request->nodeId; }
    [[nodiscard]] uint16_t           index() const { return m_request->index; }
    [[nodiscard]] uint8_t            subIndex() const { return m_request->subIndex; }
    [[nodiscard]] CO_SDO_abortCode_t abortCode() const { return m_request->abortCode; }
    //! Total size of the data which will be transferred. It is optionally used by the server.
    [[nodiscard]] size_t sizeIndicated() const { return m_request->sizeIndicated; }
    //! Number of bytes that has been sent so far.
    [[nodiscard]] size_t  sizeTransferred() const { return m_request->sizeTransferred; }
    [[nodiscard]] VarType varType() const { return m_request->varType; }

    bool cancel()
    {
        if (status() != SdoRequestStatus::Complete) {
            m_request->status = SdoRequestStatus::CancelRequested;
            return true;
        }
        return false;
    }

    std::future<std::expected<std::span<uint8_t>, CO_SDO_return_t>> future;

private:
    friend class SdoManager;
    std::shared_ptr<SdoUploadRequest> m_request;
};
}    // namespace Frasy::CanOpen

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_UPLOADER_H
