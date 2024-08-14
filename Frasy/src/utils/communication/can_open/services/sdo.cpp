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
#include <tuple>

#include <Brigerad/Core/Log.h>
#include <Brigerad/Debug/Instrumentor.h>

#include <CO_SDOclient.h>

#include <processthreadsapi.h>
#include <synchapi.h>

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
  m_odObjRecord(new OD_obj_record_t[4] {
    OD_obj_record_t {
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
    },
  })
{
    startWorkers();
}

bool SdoManager::hasUploadRequestsPending() const
{
    return !m_pendingUploadRequests.empty();
}

size_t SdoManager::uploadRequestsPending() const
{
    return m_pendingUploadRequests.size();
}

OD_entry_t SdoManager::makeSdoClientOdEntry() const
{
    return {static_cast<uint16_t>(s_sdoClientBaseAddress + m_nodeId), 0x04, ODT_REC, m_odObjRecord.get(), nullptr};
}

SdoUploadDataResult SdoManager::uploadData(
  uint16_t index, uint8_t subIndex, uint16_t sdoTimeoutTimeMs, uint8_t retries, bool isBlock)
{
    FRASY_PROFILE_FUNCTION();
    SdoUploadDataResult result;
    result.m_request = std::make_shared<SdoUploadRequest>(
      SdoRequestStatus::Queued, m_nodeId, index, subIndex, isBlock, sdoTimeoutTimeMs, retries);

    result.future = result.m_request->promise.get_future();

    if (!isAbleToMakeUploadRequests()) {
        // It's impossible to make requests without a client to talk with!
        result.m_request->abort(CO_SDO_AB_GENERAL);
        return result;
    }

    {
        // Queue the message.
        std::unique_lock lock {m_uploadLock};
        m_pendingUploadRequests.emplace(result.m_request);
        m_uploadCv.notify_all();
    }

    return result;
}

void SdoManager::startWorkers()
{
    m_stopSource = {};
    auto ret =
      CO_SDOclient_setup(m_sdoClient, m_clientInfo.cobIdClientToServer, m_clientInfo.cobIdServerToClient, m_nodeId);
    if (ret != CO_SDO_RT_ok_communicationEnd) { return; }

    m_uploadWorkerThread =
      std::jthread([this](std::stop_source source) { uploadWorkerThread(source.get_token()); }, m_stopSource);
    if (FAILED(SetThreadDescription(m_uploadWorkerThread.native_handle(), L"SDO Upload Worker"))) {
        BR_LOG_ERROR("SDO", "Unable to set thread description");
    }
    if (!SetThreadPriority(m_uploadWorkerThread.native_handle(), THREAD_PRIORITY_ABOVE_NORMAL)) {
        BR_LOG_ERROR("SDO", "Unable to set thread priority!");
    }
    m_downloadWorkerThread =
      std::jthread([this](std::stop_source source) { downloadWorkerThread(source.get_token()); }, m_stopSource);
    if (FAILED(SetThreadDescription(m_downloadWorkerThread.native_handle(), L"SDO Download Worker"))) {
        BR_LOG_ERROR("SDO", "Unable to set thread description");
    }
    if (!SetThreadPriority(m_downloadWorkerThread.native_handle(), THREAD_PRIORITY_ABOVE_NORMAL)) {
        BR_LOG_ERROR("SDO", "Unable to set thread priority!");
    }
}

void SdoManager::stopWorkers()
{
    m_stopSource.request_stop();
    CO_SDOclientClose(m_sdoClient);
}

void SdoManager::uploadWorkerThread(const std::stop_token& stopToken)
{
    m_isUploadWorkerActive = true;

    while (!stopToken.stop_requested()) {
        std::shared_ptr<SdoUploadRequest> request;
        // Get the first pending request in the queue.
        {
            std::unique_lock lock {m_uploadLock};
            m_uploadCv.wait(lock, stopToken, [this] { return !m_pendingUploadRequests.empty(); });
            if (m_pendingUploadRequests.empty()) { continue; }
            request = m_pendingUploadRequests.front();
            m_pendingUploadRequests.pop();
        }

        // If it was cancelled before it became active, ditch the request.
        if (request->status == SdoRequestStatus::CancelRequested) {
            request->cancel();
            continue;
        }

        request->status = SdoRequestStatus::OnGoing;

        CO_SDO_return_t lastReturn = CO_SDO_RT_ok_communicationEnd;
        for (int i = 0; i <= request->retries; ++i) {
            auto [handlerCode, coCode] = handleUploadRequest(*request);
            lastReturn                 = coCode;
            if (handlerCode == HandlerReturnCode::ok) {
                request->markAsComplete(std::span(request->data));
                break;
            }
            if (handlerCode == HandlerReturnCode::cancel) {
                request->cancel();
                break;
            }
            BR_APP_WARN("SDO Failure. Node {:02x}, index {:04x}, sub {:02x}, attempt {}, abort code {}, CO code {}",
                        request->nodeId,
                        request->index,
                        request->subIndex,
                        i + 1,
                        request->abortCode,
                        coCode);
        }
        if (request->status == SdoRequestStatus::OnGoing) { request->markAsComplete(std::unexpected(lastReturn)); }
    }

    m_isUploadWorkerActive = false;
}

std::tuple<SdoManager::HandlerReturnCode, CO_SDO_return_t> SdoManager::handleUploadRequest(SdoUploadRequest& request)
{
    BR_PROFILE_FUNCTION();
    // Initiate the upload.
    auto ret =
      CO_SDOclientUploadInitiate(m_sdoClient, request.index, request.subIndex, request.sdoTimeoutMs, request.isBlock);
    if (ret != CO_SDO_RT_ok_communicationEnd) { return std::make_tuple(HandlerReturnCode::error, ret); }

    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;
    auto     last  = steady_clock::now();
    uint32_t delta = 0;
    // Upload the data.
    do {
        if (request.status == SdoRequestStatus::CancelRequested) {
            return std::make_tuple(HandlerReturnCode::cancel, CO_SDO_RT_ok_communicationEnd);
        }
        uint32_t timeToSleep = 1000;    // 1ms
        uint32_t timeToSleep = 1'000;    // 1ms

        // Check and update the status of the upload.
        ret = CO_SDOclientUpload(m_sdoClient,
                                 delta,
                                 request.abortCode != CO_SDO_AB_NONE && request.status != SdoRequestStatus::OnGoing,
                                 &request.abortCode,
                                 &request.sizeIndicated,
                                 &request.sizeTransferred,
                                 &timeToSleep);

        if (ret < 0) {
            // < 0 = error code, we gotta stop.
            return std::make_tuple(HandlerReturnCode::error, ret);
        }

        if (ret == CO_SDO_RT_uploadDataBufferFull) {
            readUploadBufferIntoRequest(request);
            // We're allowed to call upload again immediately when it returns CO_SDO_RT_uploadDataBufferFull.
            timeToSleep = 0;
        }

        delta = duration_cast<microseconds>(steady_clock::now() - last).count();
        last  = steady_clock::now();
        std::this_thread::sleep_for(microseconds {timeToSleep});
            Sleep(1);
    } while (ret != CO_SDO_RT_ok_communicationEnd);

    readUploadBufferIntoRequest(request);
    BR_LOG_TRACE("SDO",
                 "Uploaded {} bytes from node {:02x}, index {:04x}, sub {:02x}",
                 request.data.size(),
                 request.nodeId,
                 request.index,
                 request.subIndex);
    return std::make_tuple(HandlerReturnCode::ok, ret);
}

void SdoManager::readUploadBufferIntoRequest(SdoUploadRequest& request)
{
    size_t read = 0;
    do {
        uint8_t tmp[16] = {};
        read            = CO_SDOclientUploadBufRead(m_sdoClient, &tmp[0], sizeof(tmp));
        request.data.insert(request.data.end(), std::begin(tmp), std::begin(tmp) + read);
    } while (read > 0);
}

void SdoManager::downloadWorkerThread(const std::stop_token& stopToken)
{
    m_isDownloadWorkerActive = true;

    while (!stopToken.stop_requested()) {
        std::shared_ptr<SdoDownloadRequest> request;
        // Get the first pending request in the queue.
        {
            std::unique_lock lock {m_downloadLock};
            m_downloadCv.wait(lock, stopToken, [this] { return !m_pendingDownloadRequests.empty(); });
            // If we got out of the wait because of the stopToken, we don't want to handle an empty object.
            if (m_pendingDownloadRequests.empty()) { continue; }
            request = m_pendingDownloadRequests.front();
            m_pendingDownloadRequests.pop();
        }

        // If it was cancelled before it became active, ditch the request.
        if (request->status == SdoRequestStatus::CancelRequested) {
            request->cancel();
            continue;
        }

        request->status = SdoRequestStatus::OnGoing;
        for (uint8_t i = 0; i <= request->retries; ++i) {
            auto [handlerCode, coCode] = handleDownloadRequest(*request);
            if (handlerCode == HandlerReturnCode::ok) {
                request->markAsComplete(coCode);
                break;
            }
            if (handlerCode == HandlerReturnCode::cancel) {
                request->cancel();
                break;
            }
            BR_APP_WARN("SDO Failure. Node {:02x}, index {:04x}, sub {:02x}, attempt {}, abort code {}, CO code {}",
                        request->nodeId,
                        request->index,
                        request->subIndex,
                        i + 1,
                        request->abortCode,
                        coCode);
            if (i == request->retries) { request->markAsComplete(coCode); }
        }
    }

    m_isDownloadWorkerActive = false;
}

std::tuple<SdoManager::HandlerReturnCode, CO_SDO_return_t> SdoManager::handleDownloadRequest(
  SdoDownloadRequest& request)
{
    BR_PROFILE_FUNCTION();
    // Initiate the download.
    auto ret = CO_SDOclientDownloadInitiate(
      m_sdoClient, request.index, request.subIndex, request.data.size(), request.sdoTimeoutMs, request.isBlock);
    if (ret != CO_SDO_RT_ok_communicationEnd) {
        return std::make_tuple(HandlerReturnCode::error, ret);
        // request.markAsComplete(ret);
        // return;
    }

    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;
    auto     last  = steady_clock::now();
    uint32_t delta = 0;
    // Upload the data.
    size_t lastTransSize = -1;
    // Write a first bunch of data into the buffer.
    size_t totalBytesWritten =
      CO_SDOclientDownloadBufWrite(m_sdoClient,
                                   request.data.data() + request.sizeTransferred,
                                   request.data.size() - std::min(request.sizeTransferred, request.data.size()));
    uint8_t tries = request.retries;
    do {
        if (request.status == SdoRequestStatus::CancelRequested) {
            return std::make_tuple(HandlerReturnCode::cancel, CO_SDO_return_t::CO_SDO_RT_ok_communicationEnd);
        }
        uint32_t timeToSleep = 1000;    // 1ms

        // Fill data if we need to send the next packet.
        size_t written = 0;
        if (request.sizeTransferred == totalBytesWritten && totalBytesWritten < request.data.size()) {
            written = CO_SDOclientDownloadBufWrite(m_sdoClient,
                                                   request.data.data() + request.sizeTransferred,
                                                   request.data.size() -
                                                     std::min(request.sizeTransferred, request.data.size()));
            totalBytesWritten += written;
            BR_LOG_DEBUG(
              s_cliTag, "Written {} more bytes to buffer. ({}/{})", written, totalBytesWritten, request.data.size());
        }
        bool bufferPartial = totalBytesWritten < request.data.size();

        ret = CO_SDOclientDownload(m_sdoClient,
                                   delta,
                                   request.abortCode != CO_SDO_AB_NONE && request.status != SdoRequestStatus::OnGoing,
                                   bufferPartial,
                                   &request.abortCode,
                                   &request.sizeTransferred,
                                   &timeToSleep);
        if (ret < 0) {
            // < 0 = error code, we gotta stop.
            return std::make_tuple(HandlerReturnCode::error, ret);
        }

        if (request.sizeTransferred != lastTransSize) {
            lastTransSize = request.sizeTransferred;
            BR_LOG_DEBUG(s_cliTag,
                         "Transfered {} bytes over to remote node. ({}/{})",
                         request.sizeTransferred,
                         request.sizeTransferred,
                         request.data.size());
        }

        delta = duration_cast<microseconds>(steady_clock::now() - last).count();
        last  = steady_clock::now();
        std::this_thread::sleep_for(microseconds {timeToSleep});
            Sleep(1);
    } while (ret != CO_SDO_RT_ok_communicationEnd);

    return std::make_tuple(HandlerReturnCode::ok, ret);
}

void SdoManager::setNodeId(uint8_t nodeId)
{
    m_nodeId = nodeId;
    startWorkers();
}

void SdoManager::setSdoClient(CO_SDOclient_t* sdoClient)
{
    m_sdoClient = sdoClient;
    startWorkers();
}

void SdoManager::removeSdoClient()
{
    m_stopSource.request_stop();
    m_sdoClient = nullptr;
}
}    // namespace Frasy::CanOpen
