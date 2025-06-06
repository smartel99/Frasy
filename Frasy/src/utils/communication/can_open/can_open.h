/**
 * @file    can_open.h
 * @author  Samuel Martel
 * @date    2024-04-22
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


#ifndef FRASY_UTILS_COMMUNICATION_CAN_OPEN_CAN_OPEN_H
#define FRASY_UTILS_COMMUNICATION_CAN_OPEN_CAN_OPEN_H

#include "utils/communication/slcan/device.h"

#include "hb_consumer.h"
#include "node.h"
#include "real_od.h"
#include "services/sdo.h"
#include "to_string.h"

#include <CANopen.h>
#include <CO_Emergency.h>
#include <CO_storage.h>

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace Frasy {
class DeviceViewer;
namespace CanOpenViewer {
class Layer;
}
}    // namespace Frasy

namespace Frasy::CanOpen {
static constexpr uint16_t s_sdoClientBaseAddress = 0x1280;

/**
 * TODO Adding nodes should perform the following actions:
 *  - A SDO client should be registered in the CANopen instance. This might require a complete re-initialization of the
 * stack?
 */
class CanOpen {
public:
    using EmergencyMessageCallback = std::function<void(const EmergencyMessage&)>;
    using Interfaces_t             = std::map<std::string, SlCan::Device>;

public:
             CanOpen();
             CanOpen(const CanOpen&)   = delete;
    CanOpen& operator=(const CanOpen&) = delete;
    //          CanOpen(CanOpen&& o) noexcept;
    // CanOpen& operator=(CanOpen&&) = default;
    ~CanOpen();

    bool addDevice(const std::string& port);
    bool removeDevice(const std::string& port);

    // void open(std::string_view port);
    void reopen();
    // void close();
    bool isOpen() const { return !m_devices.empty() && m_isRunning; }

    void reset();
    void start();
    void stop();

    std::vector<Node>& getNodes();
    Node*              getNode(uint8_t nodeId);
    Node*              addNode(uint8_t nodeId, std::string_view name = "", std::string_view edsPath = "");
    void               removeNode(uint8_t nodeId);
    void               removeNode(const Node& node);
    void               clearNodes();
    void               resetNodes() const;
    void               resetNode(uint8_t nodeId) const;
    bool               isNodeRegistered(uint8_t nodeId);
    bool               isNodeOnNetwork(uint8_t nodeId);

    void addEmergencyMessageCallback(const EmergencyMessageCallback& callback);
    void reportError(CO_EM_errorStatusBits_t kind, CO_EM_errorCode_t code, uint32_t infoCode);
    void clearError(CO_EM_errorStatusBits_t kind, CO_EM_errorCode_t code);

    CO_t* nativeHandle() { return m_co; }

    /**
     *
     * @param nodeId ID of the node.
     * @param heartbeatTimeMs Time interval (in milliseconds) between each heartbeat produced by that node.
     *
     * @note CANOpen needs to be restarted for the node to be monitored.
     */
    void setNodeHeartbeatProdTime(uint8_t nodeId, uint16_t heartbeatTimeMs);

private:
    void canOpenTask(std::stop_token stopToken);

    bool               initialInit();
    bool               runtimeInit();
    bool               initCallbacks();
    bool               initTime();
    CO_NMT_reset_cmd_t mainLoop();
    bool               deinit();

    void initNodeServices();
    void deinitNodeServices();

    CO_SDOclient_t* findSdoClientHandle(uint8_t nodeId);

    void rxReadyCallback()
    {
        m_wakeupNeeded = true;
        m_sleepOrTimeout.notify_all();
    }

    /**
     * Left there just in case it's needed, but really doesn't do anything...
     */
    void scanForDevices();

#pragma region Callbacks
    /**
     * Called whenever an error condition is received.
     *
     * @param ident CAN-ID of the emergency message. If 0, then the message was sent from this device.
     * @param errorCode
     * @param errorRegister
     * @param errorBit
     * @param infoCode
     */
    static void emRxCallback(void*          arg,
                             const uint16_t ident,
                             const uint16_t errorCode,
                             const uint8_t  errorRegister,
                             const uint8_t  errorBit,
                             uint32_t       infoCode);

    /**
     * Called whenever a HBconsumer message is received from the CAN bus.
     *
     * @param arg Pointer to instance of CanOpen
     */
    static void hbConsumerPreCallback(void* arg);
    /**
     * Called whenever the NMT satte of a remote node changes.
     *
     * @param nodeId
     * @param idx
     * @param nmtState
     * @param arg Pointer to instance of CanOpen
     */
    static void hbConsumerNmtChangedCallback(uint8_t nodeId, uint8_t idx, CO_NMT_internalState_t nmtState, void* arg);
    /**
     * Called after the first heartbeat received from a node after activating the heartbeat consumer, or after a time
     * out.
     *
     * This may be used to wake up external tasks, which would handle this and further events.
     *
     * @param nodeId
     * @param idx
     * @param arg Pointer to instance of CanOpen
     */
    static void hbConsumerStartedCallback(uint8_t nodeId, uint8_t idx, void* arg);
    /**
     * Called when the node state changes from active to timeout.
     *
     * @param nodeId
     * @param idx
     * @param arg Pointer to instance of CanOpen
     */
    static void hbConsumerTimeoutCallback(uint8_t nodeId, uint8_t idx, void* arg);
    /**
     * Called when a bootup message is received from the remote node.
     *
     * @param nodeId
     * @param idx
     * @param arg Pointer to instance of CanOpen
     */
    static void hbConsumerRemoteResetCallback(uint8_t nodeId, uint8_t idx, void* arg);

    /**
     * Called after an NMT message is received from the CAN bus.
     *
     * @param arg Pointer to instance of CanOpen
     */
    static void nmtPreCallback(void* arg);
    /**
     * Called whenever a NMT state change occurs.
     *
     * The first call to this function is made immediately, so the consumer is aware of the current NMT state.
     *
     * @param state The new NMT state.
     */
    static void nmtChangedCallback(CO_NMT_internalState_t state);

    /**
     * Called each time a RPDO message is received from the CAN bus.
     *
     * @param arg Pointer to instance of CanOpen
     */
    static void pdoPreCallback(void* arg);

    /**
     * Called after a SDO client message is received from the CAN buss or when new call without delay is necessary
     * (exchange data with own SDO server or SDO block transfer is in progress).
     *
     * @param arg Pointer to instance of CanOpen
     */
    static void sdoClientPreCallback(void* arg);

    /**
     * Called after a SDO server message is received from the CAN buss, or when new call without delay is necessary (SDO
     * block transfer is in progress).
     *
     * @param arg Pointer to instance of CanOpen
     */
    static void sdoServerPreCallback(void* arg);

    /**
     * Called after a SYNC message is received from the CAN bus.
     *
     * @param arg Pointer to instance of CanOpen
     */
    static void syncPreCallback(void* arg);

    /**
     * Called after a TIME message is received from the CAN bus.
     *
     * @param arg Pointer to instance of CanOpen
     */
    static void timePreCallback(void* arg);

    /**
     * Called after a LSS message is received from the CAN bus.
     *
     * @param arg Pointer to instance of CanOpen
     */
    static void lssMasterPreCallback(void* arg);
#pragma endregion

private:
    static constexpr const char* s_tag = "CANopen";
    std::string                  m_tag;

    friend DeviceViewer;
    friend CanOpenViewer::Layer;

    Interfaces_t m_devices;

    static constexpr auto s_nmtControlFlags = static_cast<CO_NMT_control_t>(
      CO_NMT_STARTUP_TO_OPERATIONAL | CO_NMT_ERR_ON_ERR_REG | CO_ERR_REG_GENERIC_ERR | CO_ERR_REG_COMMUNICATION);
    static constexpr uint16_t s_firstHeartbeatTime     = 500;
    static constexpr uint16_t s_sdoServerTimeoutTime   = 1000;
    static constexpr uint16_t s_sdoClientTimeoutTime   = 500;
    static constexpr bool     s_sdoClientBlockTransfer = false;
    static constexpr uint8_t  s_defaultNodeId          = 0x01;
    static constexpr uint16_t s_lssTimeout             = 50;

    static constexpr uint16_t s_cobLssSlaveId  = 0x6E1;
    static constexpr uint16_t s_cobLssMasterId = 0x6E2;

    CO_t*                             m_co = nullptr;
    CO_config_t                       m_canOpenConfig {};
    uint32_t                          m_coHeapMemoryUsed = 0;
    bool                              m_hasBeenInitOnce  = false;
    CO_storage_t                      m_storage          = {};
    std::array<CO_storage_entry_t, 1> m_storageEntries   = {
      CO_storage_entry_t {
          .addr       = &OD_PERSIST_COMM,
          .len        = sizeof(OD_PERSIST_COMM),
          .subIndexOD = 2,
          .attr       = CO_storage_cmd | CO_storage_restore,
          .filename   = {'o', 'd', '_', 'c', 'o', 'm', 'm', '.', 'p', 'e', 'r', 's', 'i', 's', 't', '\0'},
      },
    };

    std::stop_source            m_stopSource;
    std::condition_variable_any m_sleepOrTimeout;
    bool                        m_wakeupNeeded = false;
    std::jthread                m_coThread;
    bool                        m_isRunning = false;

    std::chrono::time_point<std::chrono::steady_clock> m_lastTimePoint;
    static constexpr auto                              s_autoSavePeriod = std::chrono::minutes {1};
    std::chrono::time_point<std::chrono::steady_clock> m_lastSaveTime;
    uint32_t                                           m_sleepForUs = 0;

    bool m_redLed   = false;
    bool m_greenLed = false;

    std::vector<Node>       m_nodes;
    std::vector<OD_entry_t> m_sdoClientODEntries;

    std::vector<EmergencyMessageCallback> m_emCallbacks;
};
}    // namespace Frasy::CanOpen

#endif    // FRASY_UTILS_COMMUNICATION_CAN_OPEN_CAN_OPEN_H
