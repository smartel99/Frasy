/**
 * @file    sdo_downloader.h
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


#ifndef FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_DOWNLOADER_H
#define FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_DOWNLOADER_H

#include "sdo_request_status.h"

#include <CO_SDOclient.h>

#include <cstdint>
#include <expected>
#include <future>
#include <memory>
#include <span>
#include <vector>

namespace Frasy::CanOpen {
struct SdoDownloadRequest {
    SdoRequestStatus status       = SdoRequestStatus::Unknown;
    uint8_t          nodeId       = 0;
    uint16_t         index        = 0;
    uint8_t          subIndex     = 0;
    bool             isBlock      = false;
    uint16_t         sdoTimeoutMs = 1000;

    std::vector<uint8_t> data;

    //! Number of bytes that has been sent so far.
    size_t sizeTransferred = 0;

    CO_SDO_abortCode_t            abortCode = CO_SDO_AB_NONE;
    std::promise<CO_SDO_return_t> promise;


    void markAsComplete(CO_SDO_return_t res)
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
        promise.set_value(CO_SDO_RT_endedWithClientAbort);
    }
};

struct SdoDownloadDataResult {
    [[nodiscard]] SdoRequestStatus   status() const { return m_request->status; }
    [[nodiscard]] uint8_t            nodeId() const { return m_request->nodeId; }
    [[nodiscard]] uint16_t           index() const { return m_request->index; }
    [[nodiscard]] uint8_t            subIndex() const { return m_request->subIndex; }
    [[nodiscard]] CO_SDO_abortCode_t abortCode() const { return m_request->abortCode; }
    //! Total size of the data which will be transferred.
    [[nodiscard]] size_t totalSize() const { return m_request->data.size(); }
    //! Number of bytes that has been sent so far.
    [[nodiscard]] size_t sizeTransferred() const { return m_request->sizeTransferred; }

    [[nodiscard]] const std::vector<uint8_t>& data() const { return m_request->data; }

    bool cancel()
    {
        if (status() != SdoRequestStatus::Complete) {
            m_request->status = SdoRequestStatus::CancelRequested;
            return true;
        }
        return false;
    }

    std::future<CO_SDO_return_t> future;

private:
    friend class SdoManager;
    std::shared_ptr<SdoDownloadRequest> m_request;
};
}    // namespace Frasy::CanOpen

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_DOWNLOADER_H
