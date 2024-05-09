/**
 * @file    sdo.h
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


#ifndef FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_H
#define FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_H
#include "sdo_uploader.h"

#include <CO_SDOclient.h>
#include <CO_ODinterface.h>

#include <condition_variable>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <queue>
#include <span>
#include <string>
#include <thread>

namespace Frasy::CanOpen {
class Node;

struct SdoClientInfo {
    uint32_t cobIdClientToServer = 0;
    uint32_t cobIdServerToClient = 0;
    uint8_t  serverNodId         = 0;
    uint8_t  highestSubIndex     = 3;
};

class SdoManager {
    friend Node;

public:
             SdoManager();
    explicit SdoManager(uint8_t nodeId);

    [[nodiscard]] bool   hasRequestPending() const;
    [[nodiscard]] size_t requestsPending() const;

    [[nodiscard]] OD_entry_t makeSdoClientOdEntry() const;

    /**
     * Initiates a transfer of data from a remote node to Frasy.
     *
     * @param index Index of object in the object dictionary of the remotet node.
     * @param subIndex Sub-index of object in the object dictionary of the remote node.
     * @param sdoTimeoutTimeMs Timeout time (in milliseconds) for SDO communication.
     * @param isBlock Try to initiate a block transfer.
     * @return On success, returns a future on which the entire data transfered can be received, as well as values that
     * can be used to keep track of the transfer. On failure, returns the reason.
     */
    SdoUploadDataResult uploadData(uint16_t index,
                                   uint8_t  subIndex,
                                   uint16_t sdoTimeoutTimeMs = 1000,
                                   bool     isBlock          = false);

private:
    [[nodiscard]] bool isAbleToMakeRequests() const
    {
        return m_isUploadWorkerActive && m_nodeId != s_noNodeId && m_sdoClient != nullptr;
    }

    void uploadWorkerThread(const std::stop_token& stopToken);
    void handleUploadRequest(SdoUploadRequest& request);
    void readUploadBufferIntoRequest(SdoUploadRequest& request);

    void setNodeId(uint8_t nodeId);
    void setSdoClient(CO_SDOclient_t* sdoClient);
    void removeSdoClient();

private:
    static constexpr uint8_t s_noNodeId  = 0xFF;
    uint8_t                  m_nodeId    = s_noNodeId;
    CO_SDOclient_t*          m_sdoClient = nullptr;

    SdoClientInfo   m_clientInfo;
    std::unique_ptr<OD_obj_record_t> m_odObjRecord;

    std::queue<std::shared_ptr<SdoUploadRequest>> m_pendingUploadRequests;

    std::mutex                  m_lock;
    std::condition_variable_any m_cv;
    bool                        m_isUploadWorkerActive = false;
    std::jthread                m_uploadWorkerThread;

    static constexpr const char* s_cliTag = "SDO Client";
    static constexpr const char* s_srvTag = "SDO Server";
};
}    // namespace Frasy::CanOpen

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_H
