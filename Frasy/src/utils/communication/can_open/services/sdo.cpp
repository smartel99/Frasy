/**
 * @file    sdo.cpp
 * @author  Samuel Martel
 * @date    2024-05-06
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

#include "sdo.h"

#include "../can_open.h"
#include "../to_string.h"

#include <Brigerad/Core/Log.h>
#include <Brigerad/Debug/Instrumentor.h>

#include <CO_SDOclient.h>

namespace Frasy::CanOpen {

SdoManager::SdoManager() : SdoManager(s_noNodeId)
{
}

SdoManager::SdoManager(uint8_t nodeId)
: m_nodeId(nodeId),
  m_clientInfo {
    .cobIdClientToServer = static_cast<uint32_t>(CO_CAN_ID_SDO_CLI + m_nodeId),
    .cobIdServerToClient = static_cast<uint32_t>(CO_CAN_ID_SDO_SRV + m_nodeId),
    .serverNodId         = nodeId,
    .highestSubIndex     = 3,
  },
  m_odObjRecord(new OD_obj_record_t[4] {OD_obj_record_t {
                                          .dataOrig   = &m_clientInfo.highestSubIndex,
                                          .subIndex   = 0,
                                          .attribute  = ODA_SDO_R,
                                          .dataLength = 1,
                                        },
                                        OD_obj_record_t {
                                          .dataOrig   = &m_clientInfo.cobIdClientToServer,
                                          .subIndex   = 1,
                                          .attribute  = ODA_SDO_RW | ODA_TRPDO | ODA_MB,
                                          .dataLength = 4,
                                        },
                                        OD_obj_record_t {
                                          .dataOrig   = &m_clientInfo.cobIdServerToClient,
                                          .subIndex   = 2,
                                          .attribute  = ODA_SDO_RW | ODA_TRPDO | ODA_MB,
                                          .dataLength = 4,
                                        },
                                        OD_obj_record_t {
                                          .dataOrig   = &m_clientInfo.serverNodId,
                                          .subIndex   = 3,
                                          .attribute  = ODA_SDO_RW,
                                          .dataLength = 1,
                                        }}),
  m_uploadWorkerThread(std::jthread([this](const std::stop_token& stopToken) { uploadWorkerThread(stopToken); }))
{
}

bool SdoManager::hasRequestPending() const
{
    return !m_pendingUploadRequests.empty();
}

size_t SdoManager::requestsPending() const
{
    return m_pendingUploadRequests.size();
}

OD_entry_t SdoManager::makeSdoClientOdEntry() const
{
    return {static_cast<uint16_t>(s_sdoClientBaseAddress + m_nodeId), 0x04, ODT_REC, m_odObjRecord.get(), nullptr};
}

SdoUploadDataResult SdoManager::uploadData(uint16_t index, uint8_t subIndex, uint16_t sdoTimeoutTimeMs, bool isBlock)
{
    SdoUploadDataResult result;
    result.m_request = std::make_shared<SdoUploadRequest>(
      SdoUploadRequestStatus::Queued, m_nodeId, index, subIndex, isBlock, sdoTimeoutTimeMs);

    result.future = result.m_request->promise.get_future();

    if (!isAbleToMakeRequests()) {
        // It's impossible to make requests without a client to talk with!
        result.m_request->abort(CO_SDO_AB_GENERAL);
        return result;
    }

    {
        // Queue the message.
        std::unique_lock lock {m_lock};
        m_pendingUploadRequests.emplace(result.m_request);
        m_cv.notify_all();
    }

    return result;
}

void SdoManager::uploadWorkerThread(const std::stop_token& stopToken)
{
    auto ret =
      CO_SDOclient_setup(m_sdoClient, m_clientInfo.cobIdClientToServer, m_clientInfo.cobIdServerToClient, m_nodeId);
    if (ret != CO_SDO_RT_ok_communicationEnd) { return; }

    m_isUploadWorkerActive = true;

    while (!stopToken.stop_requested()) {
        std::shared_ptr<SdoUploadRequest> request;
        // Get the first pending request in the queue.
        {
            std::unique_lock lock {m_lock};
            m_cv.wait(lock, stopToken, [this] { return !m_pendingUploadRequests.empty(); });
            if (m_pendingUploadRequests.empty()) { continue; }
            request = m_pendingUploadRequests.front();
            m_pendingUploadRequests.pop();
        }

        // If it was cancelled before it became active, ditch the request.
        if (request->status == SdoUploadRequestStatus::Cancelled) {
            request->markAsComplete(std::unexpected(CO_SDO_RT_endedWithClientAbort));
            continue;
        }

        request->status = SdoUploadRequestStatus::OnGoing;
        handleUploadRequest(*request);
    }

    m_isUploadWorkerActive = false;
    CO_SDOclientClose(m_sdoClient);
}

void SdoManager::handleUploadRequest(SdoUploadRequest& request)
{
    BR_PROFILE_FUNCTION();
    // Initiate the upload.
    auto ret =
      CO_SDOclientUploadInitiate(m_sdoClient, request.index, request.subIndex, request.sdoTimeoutMs, request.isBlock);
    if (ret != CO_SDO_RT_ok_communicationEnd) {
        request.markAsComplete(std::unexpected(ret));
        return;
    }

    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;
    auto     last  = steady_clock::now();
    uint32_t delta = 0;
    // Upload the data.
    do {
        if (request.status == SdoUploadRequestStatus::CancelRequested) {
            request.cancel();
            return;
        }
        uint32_t timeToSleep = 1'000;    // 1ms

        // Check and update the status of the upload.
        ret = CO_SDOclientUpload(m_sdoClient,
                                 delta,
                                 request.abortCode != CO_SDO_AB_NONE,
                                 &request.abortCode,
                                 &request.sizeIndicated,
                                 &request.sizeTransferred,
                                 &timeToSleep);

        if (ret < 0) {
            // < 0 = error code, we gotta stop.
            request.markAsComplete(std::unexpected(ret));
            return;
        }

        if (ret == CO_SDO_RT_uploadDataBufferFull) {
            readUploadBufferIntoRequest(request);
            // We're allowed to call upload again immediately when it returns CO_SDO_RT_uploadDataBufferFull.
            timeToSleep = 0;
        }

        delta = duration_cast<microseconds>(steady_clock::now() - last).count();
        last  = steady_clock::now();
        std::this_thread::sleep_for(microseconds {timeToSleep});
    } while (ret != CO_SDO_RT_ok_communicationEnd);

    readUploadBufferIntoRequest(request);
    BR_LOG_TRACE("SDO",
                 "Uploaded {} bytes from node {:02x}, index {:04x}, sub {:02x}",
                 request.data.size(),
                 request.nodeId,
                 request.index,
                 request.subIndex);
    request.markAsComplete(std::span {request.data});
}

void SdoManager::readUploadBufferIntoRequest(SdoUploadRequest& request)
{
    size_t read = 0;
    do {
        uint8_t tmp[16] = {};
        read            = CO_SDOclientUploadBufRead(m_sdoClient, &tmp[0], sizeof(tmp));
        request.data.insert(request.data.end(), std::begin(tmp), std::begin(tmp)+read);
    } while (read > 0);
}

void SdoManager::setNodeId(uint8_t nodeId)
{
    m_nodeId             = nodeId;
    m_uploadWorkerThread = std::jthread([this](const std::stop_token& stopToken) { uploadWorkerThread(stopToken); });
}

void SdoManager::setSdoClient(CO_SDOclient_t* sdoClient)
{
    m_sdoClient          = sdoClient;
    m_uploadWorkerThread = std::jthread([this](const std::stop_token& stopToken) { uploadWorkerThread(stopToken); });
}

void SdoManager::removeSdoClient()
{
    m_uploadWorkerThread.request_stop();
    m_sdoClient = nullptr;
}
}    // namespace Frasy::CanOpen
