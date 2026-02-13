/**
 * @file    can_open.cpp
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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "can_open.h"

#include "CO_storageWindows.h"

#include "Brigerad/Utils/dialogs/warning.h"
#include "real_od.h"

#include "../../lua/profile_events.h"

#include <Brigerad.h>

#include <array>
#include <chrono>
#include <utility>

#include <processthreadsapi.h>
#include <Windows.h>

#define EARLY_EXIT(msg, ...)                                                                                           \
    do {                                                                                                               \
        m_device.close();                                                                                              \
        BR_LOG_ERROR(m_tag, msg __VA_OPT__(, ) __VA_ARGS__);                                                           \
        return;                                                                                                        \
    } while (0)

namespace Frasy::CanOpen {
CanOpen::CanOpen()
{
    start();
}

CanOpen::~CanOpen()
{
    stop();
}

bool CanOpen::addDevice(const std::string& port)
{
    if (m_devices.contains(port)) {
        BR_LOG_WARN(m_tag, "Device on port '{}' already open!", port);
        return false;
    }

    try {
        SlCan::Device dev = SlCan::Device {port};
        dev.setRxCallbackFunc([this] { rxReadyCallback(); });
        if (dev.isOpen()) {
            m_devices[port] = std::move(dev);
            return true;
        }
    }
    catch (std::exception& e) {
        BR_LOG_ERROR(m_tag, "Error occurred while opening {}: {}", port, e.what());
    }
    return false;
}

bool CanOpen::removeDevice(const std::string& port)
{
    return m_devices.erase(port) == 1;
}

// void CanOpen::open(std::string_view port)
// {
//     if (isOpen()) { close(); }
//     m_tag  = fmt::format("{} {}", s_tag, port);
//     m_port = port;
//     try {
//         m_device = SlCan::Device {port};
//         m_device.setRxCallbackFunc([this] { rxReadyCallback(); });
//     }
//     catch (std::exception& e) {
//         BR_LOG_ERROR(m_tag, "Error occurred while opening {}: {}", port, e.what());
//         return;
//     }
//     if (!m_device.isOpen()) { BR_LOG_ERROR(m_tag, "Unable to open '{}'", port); }
//     else {
//         reset();
//     }
// }

void CanOpen::reopen()
{
    for (auto&& [port, dev] : m_devices) {
        if (dev.isOpen()) { dev.close(); }
        dev.open();
    }
}

// void CanOpen::close()
// {
//     if (!isOpen()) { return; }
//     m_device.close();
// }

void CanOpen::reset()
{
    stop();
    start();
}

void CanOpen::start()
{
    if (m_devices.empty()) {
        BR_LOG_WARN(m_tag, "Ignoring CanOpen Start with no associated devices");
        return;
    }
    m_stopSource = {};
    m_coThread   = std::jthread([this] {
        if (FAILED(SetThreadDescription(GetCurrentThread(), L"CANOpen"))) {
            BR_LOG_ERROR("CANOpen", "Unable to set thread description");
        }
        if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
            BR_LOG_ERROR("CANOpen", "Unable to set thread priority!");
        }
        addNode(0x01, "Frasy", "frasy.eds");    // TODO set the right path for the EDS.
        canOpenTask(m_stopSource.get_token());
        removeNode(0x01);
    });
}

void CanOpen::stop()
{
    if (m_stopSource.stop_possible()) { m_stopSource.request_stop(); }
    if (m_coThread.joinable()) { m_coThread.join(); }
}

#pragma region     Nodes
std::vector<Node>& CanOpen::getNodes()
{
    return m_nodes;
}

std::optional<Node*> CanOpen::getNode(uint8_t nodeId)
{
    auto it = std::ranges::find_if(m_nodes, [nodeId](const auto& node) { return node.nodeId() == nodeId; });
    return it == m_nodes.end() ? std::optional<Node*> {} : std::optional {&*it};
}

Node* CanOpen::addNode(uint8_t nodeId, std::string_view name, std::string_view edsPath)
{
    if (isNodeRegistered(nodeId)) {
        BR_LOG_ERROR(m_tag, "Node with ID 0x{:02x} already exists", nodeId);
        return nullptr;
    }

    // TODO that's fucking stupid, vector offers no pointer stability, ***especially*** in insertions!!!
    Node* node = &m_nodes.emplace_back(this, nodeId, name.empty() ? std::format("Node {}", nodeId) : name, edsPath);
    m_sdoClientODEntries.push_back(node->sdoInterface()->makeSdoClientOdEntry());
    // TODO Node should contain the OD, so its Heartbeat Producer time should be fetched from it.

    // Node will not be usable until we restart CANopen.

    return node;
}

void CanOpen::removeNode(uint8_t nodeId)
{
    std::erase_if(m_sdoClientODEntries, [nodeId](const OD_entry_t& entry) {
        return (entry.index >= s_sdoClientBaseAddress) && (entry.index - s_sdoClientBaseAddress) == nodeId;
    });
    std::erase_if(m_nodes, [nodeId](const Node& node) { return node.nodeId() == nodeId; });
}

void CanOpen::removeNode(const Node& node)
{
    std::erase_if(m_sdoClientODEntries, [nodeId = node.nodeId()](const OD_entry_t& entry) {
        return (entry.index >= s_sdoClientBaseAddress) && (entry.index - s_sdoClientBaseAddress) == nodeId;
    });
    std::erase(m_nodes, node);
}

void CanOpen::clearNodes()
{
    m_sdoClientODEntries.clear();
    m_nodes.clear();
}

void CanOpen::resetNodes() const
{
    for (const auto& node : m_nodes) {
        resetNode(node.nodeId());
    }
}

void CanOpen::resetNode(uint8_t nodeId) const
{
    CO_NMT_sendCommand(m_co->NMT, CO_NMT_RESET_NODE, nodeId);
}

bool CanOpen::isNodeRegistered(uint8_t nodeId)
{
    return std::ranges::any_of(m_nodes, [nodeId](const auto& node) { return node.nodeId() == nodeId; });
}

bool CanOpen::isNodeOnNetwork([[maybe_unused]] uint8_t nodeId)
{
    throw std::runtime_error("Not implemented");
}
#pragma endregion

void CanOpen::addEmergencyMessageCallback(const EmergencyMessageCallback& callback)
{
    if (callback) { m_emCallbacks.push_back(callback); }
}

void CanOpen::reportError(CO_EM_errorStatusBits_t kind, CO_EM_errorCode_t code, uint32_t infoCode)
{
    CO_errorReport(m_co->em, static_cast<uint8_t>(kind), static_cast<uint16_t>(code), infoCode);
}

void CanOpen::clearError(CO_EM_errorStatusBits_t kind, CO_EM_errorCode_t code)
{
    CO_errorReset(m_co->em, static_cast<uint8_t>(kind), code);
}

void CanOpen::scanForDevices()
{
    BR_PROFILE_FUNCTION();
    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;
    auto start = steady_clock::now();
    BR_LOG_INFO(m_tag, "Starting scan...");

    // No nodes must be selected at first.
    if (auto res = CO_LSSmaster_switchStateDeselect(m_co->LSSmaster); res != CO_LSSmaster_OK) {
        BR_LOG_ERROR(m_tag, "Unable to deselect LSS slaves: ({:X}) {}", std::to_underlying(res), res);
        return;
    }

    std::vector<CO_LSS_address_t> nodesFound;
    auto                          last  = steady_clock::now();
    uint32_t                      delta = 0;

    size_t                  passes  = 0;
    CO_LSSmaster_return_t   scanRes = CO_LSSmaster_WAIT_SLAVE;
    CO_LSSmaster_fastscan_t scanPass {
      .scan  = {CO_LSSmaster_FS_SCAN, CO_LSSmaster_FS_SCAN, CO_LSSmaster_FS_SKIP, CO_LSSmaster_FS_SCAN},
      .match = {},
      .found = {},
    };
    while (isOpen() && scanRes == CO_LSSmaster_WAIT_SLAVE) {
        ++passes;
        scanRes = CO_LSSmaster_IdentifyFastscan(m_co->LSSmaster, delta, &scanPass);

        if (scanRes != CO_LSSmaster_WAIT_SLAVE) {
            BR_LOG_ERROR(m_tag, "Error while scanning: ({:X}) {}", std::to_underlying(scanRes), scanRes);
            break;
        }

        delta = static_cast<uint32_t>(duration_cast<microseconds>(steady_clock::now() - last).count());
        last  = steady_clock::now();
        std::this_thread::sleep_for(microseconds {100});
    }

    auto taken = static_cast<float>(duration_cast<milliseconds>(steady_clock::now() - start).count()) / 1000.0f;
    BR_LOG_INFO(m_tag, "Scan complete! Found {} nodes in {} seconds and {} passes", nodesFound.size(), taken, passes);
    BR_LOG_INFO(m_tag, "Found: {::x}", scanPass.found.addr);
}

void CanOpen::setNodeHeartbeatProdTime(uint8_t nodeId, uint16_t heartbeatTimeMs)
{
    // Linear-search for the first free slot in 0x1060 entry searching for duplicates,
    // Set the time in that entry,
    // Increment the number of producers.
    // If heartbeatTimeMs is 0, the entry needs to be deleted.

#define ENTRY_ID(entry) (uint8_t)(((entry)&0x00FF0000) >> 16)
    if (heartbeatTimeMs == 0) {
        size_t pos = 0;
        // Find the entry:
        for (pos = 0; pos < OD_PERSIST_COMM.x1016_consumerHeartbeatTime_sub0; ++pos) {
            if (ENTRY_ID(OD_PERSIST_COMM.x1016_consumerHeartbeatTime[pos]) == nodeId) {
                BR_LOG_DEBUG(m_tag, "Cleared heartbeat producer time for node {}", nodeId);
                OD_PERSIST_COMM.x1016_consumerHeartbeatTime[pos] = 0;
                // Fill the void left by this violent removal.
                --OD_PERSIST_COMM.x1016_consumerHeartbeatTime_sub0;
                std::swap(
                  OD_PERSIST_COMM.x1016_consumerHeartbeatTime[pos],
                  OD_PERSIST_COMM.x1016_consumerHeartbeatTime[OD_PERSIST_COMM.x1016_consumerHeartbeatTime_sub0]);
                break;
            }
        }
    }
    else {
        bool updatedEntry = false;
        bool heartbeatSet = false;
        for (uint32_t& entry : OD_PERSIST_COMM.x1016_consumerHeartbeatTime) {
            if (ENTRY_ID(entry) == nodeId) {
                updatedEntry = true;
                heartbeatSet = true;
                entry        = (entry & 0xFFFF0000) | heartbeatTimeMs;
                break;
            }
            if (entry == 0) {
                // New entry.
                heartbeatSet = true;
                entry        = static_cast<uint32_t>(nodeId) << 16 | heartbeatTimeMs;
                ++OD_PERSIST_COMM.x1016_consumerHeartbeatTime_sub0;
                break;
            }
        }
        if (heartbeatSet) {
            BR_LOG_DEBUG(
              m_tag, "{} node {}'s heartbeat time to {} ms", updatedEntry ? "Updated" : "Set", nodeId, heartbeatTimeMs);
        }
        else {
            BR_LOG_ERROR(m_tag, "Unable to find a producer slot for node {}'s heartbeat!", nodeId);
        }
    }
#undef ENTRY_ID
}

void CanOpen::canOpenTask(std::stop_token stopToken)
{
    if (!initialInit()) {
        for (auto&& [port, dev] : m_devices) {
            dev.close();
        }
        return;
    }

    CO_NMT_reset_cmd_t reset = CO_RESET_NOT;
    while (!stopToken.stop_requested() && reset != CO_RESET_APP && reset != CO_RESET_QUIT) {
        // Complete the init,
        if (!runtimeInit()) { break; }

        m_isRunning = true;
        // While loop,
        while (reset == CO_RESET_NOT && !stopToken.stop_requested()) {
            FRASY_PROFILE_SCOPE("CANopen task");
            // A conditional variable is used here in place of std::this_thread::sleep_for in order to obtain a delay
            // that can be cancelled from elsewhere.
            // Although we're asking for a sleep duration of a minimum of 1 microsecond, we're actually getting a 1
            // millisecond delay. The delay gets cancelled upon reception of a CAN message.
            // Since std::condition_variable requires a lock, we give it a fake lock so it's happy and we're not losing
            // any time acquiring and releasing it.
            struct {
                void lock() {}

                void unlock() {}
            } fakeLock;
            m_sleepOrTimeout.wait_for(
              fakeLock, stopToken, std::chrono::microseconds(std::max(m_sleepForUs, 1U)), [this] {
                  if (m_wakeupNeeded) {
                      m_wakeupNeeded = false;
                      return true;
                  }
                  return false;
              });
            reset = mainLoop();
        }
    }
    // Terminate CANopen,
    deinit();
    m_isRunning = false;
}

#pragma region Initialization
bool           CanOpen::initialInit()
{
    // Initialize CANopen.
    OD_INIT_CONFIG(m_canOpenConfig);
    m_canOpenConfig.CNT_SDO_CLI = static_cast<uint8_t>(m_nodes.size());
    m_canOpenConfig.ENTRY_H1280 = m_sdoClientODEntries.data();

    // TODO these fields should be determined based on a loaded environment.
#if CO_CONFIG_LEDS & CO_CONFIG_LEDS_ENABLE
    m_canOpenConfig.CNT_LEDS = 1;
#endif
#if (CO_CONFIG_LSS) & CO_CONFIG_LSS_SLAVE
    // TODO this is the main field that should be changed. It is based on the number of nodes.
    m_canOpenConfig.CNT_LSS_SLV = 1;
#endif
#if (CO_CONFIG_LSS) & CO_CONFIG_LSS_MASTER
    m_canOpenConfig.CNT_LSS_MST = 1;
#endif
#if (CO_CONFIG_GTW) & CO_CONFIG_GTW_ASCII
    m_canOpenConfig.CNT_GTWA = 1;
#endif
#if (CO_CONFIG_TRACE) & CO_CONFIG_TRACE_ENABLE
    m_canOpenConfig.CNT_TRACE = 1;
#endif

    // Allocate memory for CANopen objects.
    m_co = CO_new(&m_canOpenConfig, &m_coHeapMemoryUsed);
    if (m_co == nullptr) {
        BR_LOG_ERROR(m_tag, "Unable to create CANopen context");
        return false;
    }
    BR_LOG_INFO(m_tag, "Created CANopen with {} bytes used", m_coHeapMemoryUsed);

    uint32_t         storageInitError = 0;
    CO_ReturnError_t err              = CO_storageWindows_init(&m_storage,
                                                  m_co->CANmodule,
                                                  OD_ENTRY_H1010_storeParameters,
                                                  OD_ENTRY_H1011_restoreDefaultParameters,
                                                  m_storageEntries.data(),
                                                  static_cast<uint8_t>(m_storageEntries.size()),
                                                  &storageInitError);
    if (err == CO_ERROR_DATA_CORRUPT) { BR_LOG_WARN(m_tag, "Persistence data corrupted!"); }
    else if (err != CO_ERROR_NO) {
        const char* filename =
          storageInitError < m_storageEntries.size() ? &m_storageEntries[storageInitError].filename[0] : "???";
        BR_LOG_ERROR(m_tag, "Error with storage '{}'", filename);
        return false;
    }

    return true;
}

bool CanOpen::runtimeInit()
{
    if (!m_hasBeenInitOnce) {
        CO_LOCK_OD(m_co->CANmodule);
        m_co->CANmodule->CANnormal = false;
        CO_UNLOCK_OD(m_co->CANmodule);
    }

    deinitNodeServices();

    // Enter CAN configuration mode.
    CO_CANsetConfigurationMode(&m_devices);
    CO_CANmodule_disable(m_co->CANmodule);

    // Initialize CANopen.
    auto err = CO_CANinit(m_co, &m_devices, 0 /* bit rate not used*/);
    if (err != CO_ERROR_NO) {
        BR_LOG_ERROR(m_tag, "CANopen error in CO_CANinit(): ({}) {}", std::to_underlying(err), err);
        return false;
    }

    uint32_t errInfo = 0;
    err              = CO_CANopenInit(m_co,
                         // CANopen object.
                         nullptr,
                         // alternate NMT handle.
                         nullptr,
                         // alternate emergency handle, might be required.
                         OD,
                         // TODO: Object dictionary must be dynamically built from the loaded environment.
                         nullptr,
                         // Optional OD_statusBits.
                         s_nmtControlFlags,
                         s_firstHeartbeatTime,
                         s_sdoServerTimeoutTime,
                         s_sdoClientTimeoutTime,
                         s_sdoClientBlockTransfer,
                         s_defaultNodeId,
                         &errInfo);
    if (err != CO_ERROR_NO && err != CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
        if (err == CO_ERROR_OD_PARAMETERS) { BR_LOG_ERROR(m_tag, "Error in Object Dictionary entry {:#08x}", errInfo); }
        else {
            BR_LOG_ERROR(m_tag, "CANopen error in CO_CANopenInit(): ({}) {}", std::to_underlying(err), err);
        }
        return false;
    }
    CO_LSSmaster_changeTimeout(m_co->LSSmaster, s_lssTimeout);

    // Initialize part of threadMain and the various callbacks.
    if (!initCallbacks()) { return false; }
    if (!initTime()) { return false; }

    err = CO_CANopenInitPDO(m_co, m_co->em, OD, s_defaultNodeId, &errInfo);
    if (err != CO_ERROR_NO && err != CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
        if (err == CO_ERROR_OD_PARAMETERS) { BR_LOG_ERROR(m_tag, "Error in Object Dictionary entry {:#08x}", errInfo); }
        else {
            BR_LOG_ERROR(m_tag, "CANopen error in CO_CANopenInitPDO(): ({}) {}", std::to_underlying(err), err);
        }
        return false;
    }

    // Start the CAN.
    CO_CANsetNormalMode(m_co->CANmodule);

    BR_LOG_INFO(m_tag, "CANOpen initialized and running!");

    initNodeServices();

    return true;
}

bool CanOpen::initCallbacks()
{
    CO_EM_initCallbackRx(m_co->em, this, &emRxCallback);

    CO_HBconsumer_initCallbackPre(m_co->HBcons, this, &hbConsumerPreCallback);
    // TODO These might need to be called per-node?
    CO_HBconsumer_initCallbackNmtChanged(m_co->HBcons, 0, this, &hbConsumerNmtChangedCallback);
    CO_HBconsumer_initCallbackHeartbeatStarted(m_co->HBcons, 0, this, &hbConsumerStartedCallback);
    CO_HBconsumer_initCallbackTimeout(m_co->HBcons, 0, this, &hbConsumerTimeoutCallback);
    CO_HBconsumer_initCallbackRemoteReset(m_co->HBcons, 0, this, &hbConsumerRemoteResetCallback);

    CO_NMT_initCallbackPre(m_co->NMT, this, &nmtPreCallback);
    CO_NMT_initCallbackChanged(m_co->NMT, &nmtChangedCallback);

    CO_RPDO_initCallbackPre(m_co->RPDO, this, &pdoPreCallback);

    CO_SDOclient_initCallbackPre(m_co->SDOclient, this, &sdoClientPreCallback);

    CO_SDOserver_initCallbackPre(m_co->SDOserver, this, &sdoServerPreCallback);

    CO_TIME_initCallbackPre(m_co->TIME, this, &timePreCallback);

    CO_LSSmaster_initCallbackPre(m_co->LSSmaster, this, &lssMasterPreCallback);
    BR_LOG_INFO(m_tag, "Callback initialized");
    return true;
}

bool CanOpen::initTime()
{
    if (m_hasBeenInitOnce) { return true; }
    m_hasBeenInitOnce = true;

    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::system_clock;
    auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    static constexpr auto msInADay = 24 * 60 * 60 * 1000;
    auto                  days     = static_cast<uint16_t>(now / msInADay);
    days -= 5113;                                       // Difference between POSIX epoch and CANopen epoch.
    auto ms = static_cast<uint32_t>(now % msInADay);    // Number of ms since midnight today.

    CO_TIME_set(m_co->TIME, ms, days, 1000);

    BR_LOG_INFO(m_tag, "Time set: {} days, {} ms", days, ms);
    return true;
}
#pragma endregion

CO_NMT_reset_cmd_t CanOpen::mainLoop()
{
    CO_CANpollReceive(m_co->CANmodule);

    using std::chrono::duration_cast;
    using std::chrono::microseconds;
    using std::chrono::steady_clock;
    auto now        = steady_clock::now();
    auto deltaUs    = duration_cast<microseconds>(now - m_lastTimePoint);
    m_lastTimePoint = now;

    if (now - m_lastSaveTime > s_autoSavePeriod) {
        m_lastSaveTime = now;
        auto ret       = CO_storageWindows_auto_process(&m_storage, false);
        if (ret != 0) { BR_LOG_ERROR(m_tag, "Unable to save persistence data on fields: {:08x}", ret); }
    }
    auto cmd   = CO_process(m_co, true, static_cast<uint32_t>(deltaUs.count()), &m_sleepForUs);
    m_greenLed = CO_LED_GREEN(m_co->LEDs, CO_LED_CANopen);
    m_redLed   = CO_LED_RED(m_co->LEDs, CO_LED_CANopen);

    return cmd;
}

bool CanOpen::deinit()
{
    bool     success = true;
    uint32_t ret     = CO_storageWindows_auto_process(&m_storage, true);
    if (ret != 0) {
        success = false;
        BR_LOG_ERROR(m_tag, "Unable to save persistence data on fields: {:08x}", ret);
    }

    // Remove all the nodes' hooks to CAN open, for they are now invalid.
    deinitNodeServices();

    CO_CANsetConfigurationMode(&m_devices);
    CO_delete(m_co);
    m_co = nullptr;

    BR_LOG_INFO(m_tag, "CANopen de-initialized!");
    return success;
}

void CanOpen::initNodeServices()
{
    for (auto&& node : m_nodes) {
        node.setHbConsumer(m_co->HBcons);
        node.setSdoClient(findSdoClientHandle(node.nodeId()));
    }
}

void CanOpen::deinitNodeServices()
{
    for (auto&& node : m_nodes) {
        node.removeHbConsumer();
        node.removeSdoClient();
    }
}

CO_SDOclient_t* CanOpen::findSdoClientHandle(uint8_t nodeId)
{
    auto* sdoClientPtr = m_co->SDOclient;
    for (uint8_t i = 0; i < m_co->config->CNT_SDO_CLI; i++) {
        if (sdoClientPtr->nodeIDOfTheSDOServer == nodeId) {
            BR_LOG_DEBUG(m_tag, "Found SDO client handle for node {:02x} at position {}!", nodeId, i);
            return sdoClientPtr;
        }
        ++sdoClientPtr;
    }
    BR_LOG_ERROR(m_tag, "No SDO client found for node {:02x}!", nodeId);
    return nullptr;
}

#pragma region Callbacks
void           CanOpen::emRxCallback(void*          arg,
                           const uint16_t ident,
                           const uint16_t errorCode,
                           const uint8_t  errorRegister,
                           const uint8_t  errorBit,
                           uint32_t       infoCode)
{
    BR_CORE_ASSERT(arg != nullptr, "arg is null in emRxCallback");
    BR_PROFILE_FUNCTION();
    EmergencyMessage emergencyMessage {
      static_cast<uint8_t>(ident & 0x7F),
      static_cast<CO_EM_errorCode_t>(errorCode),
      static_cast<CO_errorRegister_t>(errorRegister),
      static_cast<CO_EM_errorStatusBits_t>(errorBit),
      infoCode,
      EmergencyMessage::timestamp_t::clock::now(),
      true,
    };

    // TODO propagate errors to orchestrator.

    CanOpen* that = static_cast<CanOpen*>(arg);

    if (that == nullptr) {
        if (!emergencyMessage.isCritical()) {
            // not that much of a big deal I guess...
            return;
        }
        // Panic the fuck out of this aaaaaaa
        Brigerad::warningDialog(
          "CANOpen doesn't", "Received emergency message, but there's no CANOpen instance!\n\r{}", emergencyMessage);
    }

    for (auto&& cb : that->m_emCallbacks) {
        cb(emergencyMessage);
    }

    // The source is us?
    if (emergencyMessage.nodeId == 0) {
        auto maybeNode = that->getNode(0x01);
        if (maybeNode.has_value()) { (*maybeNode)->addEmergency(emergencyMessage); }
        return;
    }

    auto it = std::ranges::find_if(
      that->m_nodes, [&emergencyMessage](const auto& node) { return node.nodeId() == emergencyMessage.nodeId; });

    if (it == that->m_nodes.end()) {
        if (!emergencyMessage.isCritical()) { return; }
        // Panic the fuck out of this aaaaaaa
        Brigerad::warningDialog(
          "EMERGENCY", "Received emergency message from unregistered node\n\r{}", emergencyMessage);
        return;
    }

    it->addEmergency(emergencyMessage);
}

void CanOpen::hbConsumerPreCallback(void* arg)
{
    FRASY_PROFILE_FUNCTION();
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in hbConsumerPreCallback");
    [[maybe_unused]] CanOpen* that = static_cast<CanOpen*>(arg);
}

void CanOpen::hbConsumerNmtChangedCallback(uint8_t nodeId, uint8_t idx, CO_NMT_internalState_t nmtState, void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in hbConsumerNmtChangedCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_DEBUG(that->m_tag, "NMT state changed to {} on node {:#02x} (idx: {})", nmtState, nodeId, idx);
}

void CanOpen::hbConsumerStartedCallback(uint8_t nodeId, uint8_t idx, void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in hbConsumerStartedCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_DEBUG(that->m_tag, "Heartbeat consumer started on node {:#02x} (idx: {})", nodeId, idx);
}

void CanOpen::hbConsumerTimeoutCallback(uint8_t nodeId, uint8_t idx, void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in hbConsumerTimeoutCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_WARN(that->m_tag, "Heartbeat timed out on node {:#02x} (idx: {})", nodeId, idx);
}

void CanOpen::hbConsumerRemoteResetCallback(uint8_t nodeId, uint8_t idx, void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in hbConsumerRemoteResetCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_DEBUG(that->m_tag, "Remote node {:#02x} (idx: {}) reset!", nodeId, idx);
}

void CanOpen::nmtPreCallback(void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in nmtPreCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_DEBUG(that->m_tag, "NMT pre callback");
}

void CanOpen::nmtChangedCallback(CO_NMT_internalState_t state)
{
    BR_LOG_DEBUG(s_tag, "NMT state changed to {}", state);
}

void CanOpen::pdoPreCallback(void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in pdoPreCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_DEBUG(that->m_tag, "PDO pre callback");
}

void CanOpen::sdoServerPreCallback(void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in sdoServerPreCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_DEBUG(that->m_tag, "SDO Server pre callback");
}

void CanOpen::lssMasterPreCallback(void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in lssMasterPreCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_TRACE(that->m_tag, "LSS Master pre callback");
}

void CanOpen::sdoClientPreCallback(void* arg)
{
    FRASY_PROFILE_FUNCTION();
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in sdoClientPreCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_DEBUG(that->m_tag, "SDO Client pre callback");
}

void CanOpen::syncPreCallback(void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in syncPreCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_DEBUG(that->m_tag, "SYNC pre callback");
}

void CanOpen::timePreCallback(void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in timePreCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_DEBUG(that->m_tag, "TIME pre callback");
}
#pragma endregion


#undef EARLY_EXIT
}    // namespace Frasy::CanOpen
