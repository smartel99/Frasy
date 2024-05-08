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
class CanOpen;

class SdoManager {
    friend CanOpen;

public:
    SdoManager();
    [[nodiscard]] bool   hasRequestPending() const;
    [[nodiscard]] size_t requestsPending() const;

    /**
     * Initiates a transfer of data from a remote node to Frasy.
     *
     * @param nodeId ID of the remote node.
     * @param index Index of object in the object dictionary of the remotet node.
     * @param subIndex Sub-index of object in the object dictionary of the remote node.
     * @param sdoTimeoutTimeMs Timeout time (in milliseconds) for SDO communication.
     * @param isBlock Try to initiate a block transfer.
     * @return On success, returns a future on which the entire data transfered can be received, as well as values that
     * can be used to keep track of the transfer. On failure, returns the reason.
     */
    SdoUploadDataResult uploadData(
      uint8_t nodeId, uint16_t index, uint8_t subIndex, uint16_t sdoTimeoutTimeMs = 1000, bool isBlock = false);

private:
    void uploadWorkerThread(std::stop_token stopToken);
    void handleUploadRequest(SdoUploadRequest& request);
    void readUploadBufferIntoRequest(SdoUploadRequest& request);

    void setSdoClient(CO_SDOclient_t* sdoClient) { m_sdoClient = sdoClient; }
    void removeSdoClient() { m_sdoClient = nullptr; }

private:
    CO_SDOclient_t* m_sdoClient;

    std::queue<std::shared_ptr<SdoUploadRequest>> m_pendingUploadRequests;

    std::mutex                  m_lock;
    std::condition_variable_any m_cv;
    std::jthread                m_uploadWorkerThread;

    static constexpr const char* s_cliTag = "SDO Client";
    static constexpr const char* s_srvTag = "SDO Server";
};

class SdoInterface {
public:
    SdoInterface() = default;
    SdoInterface(SdoManager* sdo, uint8_t nodeId) : m_sdo(sdo), m_nodeId(nodeId) {}

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
                                   bool     isBlock          = false)
    {
        if (m_sdo == nullptr) { throw std::runtime_error("SdoInterface not initialized"); }
        return m_sdo->uploadData(m_nodeId, index, subIndex, sdoTimeoutTimeMs, isBlock);
    }

private:
    SdoManager* m_sdo    = nullptr;
    uint8_t     m_nodeId = 0;
};
}    // namespace Frasy::CanOpen

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_SERVICES_SDO_H
