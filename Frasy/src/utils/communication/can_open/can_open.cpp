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
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */
#include "can_open.h"

#include "CO_storageWindows.h"

#include "real_od.h"

#include <Brigerad.h>

#include <array>
#include <chrono>

#define EARLY_EXIT(msg, ...)                                                                                           \
    do {                                                                                                               \
        m_device.close();                                                                                              \
        BR_LOG_ERROR(m_tag, msg __VA_OPT__(, ) __VA_ARGS__);                                                           \
        return;                                                                                                        \
    } while (0)

namespace Frasy::CanOpen {
namespace {
std::string_view canOpenErrorToStr(CO_ReturnError_t err)
{
    using namespace std::string_view_literals;
    switch (err) {
        case CO_ERROR_NO: return "No error"sv;
        case CO_ERROR_ILLEGAL_ARGUMENT: return "Illegal Argument"sv;
        case CO_ERROR_OUT_OF_MEMORY: return "Out of Memory"sv;
        case CO_ERROR_TIMEOUT: return "Timeout"sv;
        case CO_ERROR_ILLEGAL_BAUDRATE: return "Illegal Baudrate"sv;
        case CO_ERROR_RX_OVERFLOW: return "RX Overflow"sv;
        case CO_ERROR_RX_PDO_OVERFLOW: return "RX PDO Overflow"sv;
        case CO_ERROR_RX_MSG_LENGTH: return "RX MSG Length"sv;
        case CO_ERROR_RX_PDO_LENGTH: return "RX PDO Length"sv;
        case CO_ERROR_TX_OVERFLOW: return "TX Overflow"sv;
        case CO_ERROR_TX_PDO_WINDOW: return "TX PDO Window"sv;
        case CO_ERROR_TX_UNCONFIGURED: return "TX Uncofigured"sv;
        case CO_ERROR_OD_PARAMETERS: return "OD Parameters"sv;
        case CO_ERROR_DATA_CORRUPT: return "Data Corrupt"sv;
        case CO_ERROR_CRC: return "CRC Error"sv;
        case CO_ERROR_TX_BUSY: return "TX Busy"sv;
        case CO_ERROR_WRONG_NMT_STATE: return "Wrong NMT State"sv;
        case CO_ERROR_SYSCALL: return "Syscall Error"sv;
        case CO_ERROR_INVALID_STATE: return "Invalid State"sv;
        case CO_ERROR_NODE_ID_UNCONFIGURED_LSS: return "Unconfigured LSS Node ID"sv;
        default: return "Unknown"sv;
    }
}

std::string_view canOpenNmtStateToStr(CO_NMT_internalState_t state)
{
    using namespace std::string_view_literals;
    switch (state) {
        case CO_NMT_INITIALIZING: return "Initializing"sv;
        case CO_NMT_PRE_OPERATIONAL: return "Pre-Operational"sv;
        case CO_NMT_OPERATIONAL: return "Operational"sv;
        case CO_NMT_STOPPED: return "Stopped"sv;
        case CO_NMT_UNKNOWN:
        default: return "Unknown"sv;
    }
}

std::string_view canOpenLssMasterReturnToStr(CO_LSSmaster_return_t val)
{
    using namespace std::string_view_literals;
    switch (val) {
        case CO_LSSmaster_SCAN_FINISHED: return "Scan Finished"sv;
        case CO_LSSmaster_WAIT_SLAVE: return "Wait Slave"sv;
        case CO_LSSmaster_OK: return "OK"sv;
        case CO_LSSmaster_TIMEOUT: return "Timeout"sv;
        case CO_LSSmaster_ILLEGAL_ARGUMENT: return "Illegal Argument"sv;
        case CO_LSSmaster_INVALID_STATE: return "Invalid State"sv;
        case CO_LSSmaster_SCAN_NOACK: return "No ACK from scan"sv;
        case CO_LSSmaster_SCAN_FAILED: return "Scan Failed"sv;
        case CO_LSSmaster_OK_ILLEGAL_ARGUMENT: return "OK - Illegal Argument"sv;
        case CO_LSSmaster_OK_MANUFACTURER: return "OK - Manufacturer"sv;
        default: return "Unknown"sv;
    }
}
}    // namespace


CanOpen::CanOpen()
{
    start();
}

CanOpen::CanOpen(CanOpen&& o) noexcept
: m_tag(std::move(o.m_tag)),
  m_port(std::move(o.m_port)),
  m_device(std::move(o.m_device)),
  m_co(std::move(o.m_co)),
  m_canOpenConfig(std::move(o.m_canOpenConfig)),
  m_coHeapMemoryUsed(std::move(o.m_coHeapMemoryUsed)),
  m_hasBeenInitOnce(std::move(o.m_hasBeenInitOnce)),
  m_storage(std::move(o.m_storage)),
  m_storageEntries(std::move(o.m_storageEntries)),
  m_stopSource(std::move(o.m_stopSource)),
  m_coThread(std::move(o.m_coThread)),
  m_lastTimePoint(std::move(o.m_lastTimePoint)),
  m_lastSaveTime(std::move(o.m_lastSaveTime)),
  m_redLed(std::move(o.m_redLed)),
  m_greenLed(std::move(o.m_greenLed)),
  m_nodes(std::move(o.m_nodes))
{
    for (auto&& nodes : m_nodes) {
        nodes.m_canOpen = this;
    }
}

CanOpen::~CanOpen()
{
    stop();
    if (isOpen()) { close(); }
}

void CanOpen::open(std::string_view port)
{
    if (isOpen()) { close(); }
    m_tag  = fmt::format("{} {}", s_tag, port);
    m_port = port;
    try {
        m_device = SlCan::Device {port};
    }
    catch (std::exception& e) {
        BR_LOG_ERROR(m_tag, "Error occurred while opening {}: {}", port, e.what());
        return;
    }
    if (!m_device.isOpen()) {
        BR_LOG_ERROR(m_tag, "Unable to open '{}'", port);
        return;
    }
}

void CanOpen::close()
{
    if (!isOpen()) { return; }
    m_device.close();
}

void CanOpen::reset()
{
    stop();
    start();
}

void CanOpen::start()
{
    m_stopSource = {};
    m_coThread   = std::jthread([this] { canOpenTask(m_stopSource.get_token()); });
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

Node* CanOpen::getNode(uint8_t nodeId)
{
    auto it = std::ranges::find_if(m_nodes, [nodeId](const auto& node) { return node.nodeId() == nodeId; });
    return it == m_nodes.end() ? nullptr : &*it;
}

Node* CanOpen::addNode(uint8_t nodeId, std::string_view name, std::string_view edsPath)
{
    if (isNodeRegistered(nodeId)) {
        BR_LOG_ERROR(m_tag, "Node with ID 0x{:02x} already exists", nodeId);
        return nullptr;
    }

    return &m_nodes.emplace_back(this, nodeId, name.empty() ? std::format("Node {:02x}", nodeId) : name, edsPath);
}

void CanOpen::removeNode(uint8_t nodeId)
{
    std::erase_if(m_nodes, [nodeId](const Node& node) { return node.nodeId() == nodeId; });
}

void CanOpen::removeNode(const Node& node)
{
    std::erase(m_nodes, node);
}

void CanOpen::clearNodes()
{
    m_nodes.clear();
}

bool CanOpen::isNodeRegistered(uint8_t nodeId)
{
    return std::ranges::any_of(m_nodes, [nodeId](const auto& node) { return node.nodeId() == nodeId; });
}

bool CanOpen::isNodeOnNetwork(uint8_t nodeId)
{
    throw std::runtime_error("Not implemented");
}
#pragma endregion

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
        BR_LOG_ERROR(m_tag, "Unable to deselect LSS slaves: ({:X}) {}", res, canOpenLssMasterReturnToStr(res));
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
            BR_LOG_ERROR(m_tag, "Error while scanning: ({:X}) {}", scanRes, canOpenLssMasterReturnToStr(scanRes));
            break;
        }

        delta = duration_cast<microseconds>(steady_clock::now() - last).count();
        last  = steady_clock::now();
        std::this_thread::sleep_for(microseconds {100});
    }

    auto taken = static_cast<float>(duration_cast<milliseconds>(steady_clock::now() - start).count()) / 1000.0f;
    BR_LOG_INFO(m_tag, "Scan complete! Found {} nodes in {} seconds and {} passes", nodesFound.size(), taken, passes);
    BR_LOG_INFO(m_tag, "Found: {:x}", fmt::join(scanPass.found.addr, ", "));
}

void CanOpen::canOpenTask(std::stop_token stopToken)
{
    if (!initialInit(m_port)) {
        m_device.close();
        return;
    }
    CO_NMT_reset_cmd_t reset = CO_RESET_NOT;
    while (!stopToken.stop_requested() && reset != CO_RESET_APP && reset != CO_RESET_QUIT) {
        // Complete the init,
        if (!runtimeInit()) { break; }
        // While loop,
        while (reset == CO_RESET_NOT && !stopToken.stop_requested()) {
            reset = mainLoop();
            std::this_thread::sleep_for(std::chrono::microseconds(m_sleepForUs));
        }
    }
    // Terminate CANopen,
    deinit();
    // Close the device.
    m_device.close();
}

#pragma region Initialization
bool           CanOpen::initialInit(std::string_view port)
{
    // Initialize CANopen.
    OD_INIT_CONFIG(m_canOpenConfig);
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
                                                  m_storageEntries.size(),
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

    // Enter CAN configuration mode.
    CO_CANsetConfigurationMode(&m_device);
    CO_CANmodule_disable(m_co->CANmodule);

    // Initialize CANopen.
    auto err = CO_CANinit(m_co, &m_device, 0 /* bit rate not used*/);
    if (err != CO_ERROR_NO) {
        BR_LOG_ERROR(m_tag, "CANopen error in CO_CANinit(): ({}) {}", err, canOpenErrorToStr(err));
        return false;
    }

    uint32_t errInfo = 0;
    err              = CO_CANopenInit(m_co,       // CANopen object.
                         nullptr,    // alternate NMT handle.
                         nullptr,    // alternate emergency handle, might be required.
                         OD,         // Object dictionary
                         nullptr,    // Optional OD_statusBits.
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
            BR_LOG_ERROR(m_tag, "CANopen error in CO_CANopenInit(): ({}) {}", err, canOpenErrorToStr(err));
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
            BR_LOG_ERROR(m_tag, "CANopen error in CO_CANopenInitPDO(): ({}) {}", err, canOpenErrorToStr(err));
        }
        return false;
    }

    // Start the CAN.
    CO_CANsetNormalMode(m_co->CANmodule);

    BR_LOG_INFO(m_tag, "CANOpen initialized and running!");

    return true;
}

bool CanOpen::initCallbacks()
{
    CO_EM_initCallbackRx(m_co->em, &emRxCallback);

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
        if (ret != 0) { BR_LOG_ERROR(m_tag, "Unable to save persistance data on fields: {:08x}", ret); }
    }
    auto cmd   = CO_process(m_co, true, deltaUs.count(), &m_sleepForUs);
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
        BR_LOG_ERROR(m_tag, "Unable to save persistance data on fields: {:08x}", ret);
    }

    CO_CANsetConfigurationMode(&m_device);
    CO_delete(m_co);
    m_co = nullptr;

    BR_LOG_INFO(m_tag, "CANopen de-initialized!");
    return success;
}

#pragma region Callbacks
void           CanOpen::emRxCallback(const uint16_t ident,
                           const uint16_t errorCode,
                           const uint8_t  errorRegister,
                           const uint8_t  errorBit,
                           uint32_t       infoCode)
{
    uint8_t nodeId = static_cast<uint8_t>(ident) & 0x7F;
    BR_LOG_DEBUG(s_tag,
                 "Emergency message received from {:#02x}:"
                 "\n\r\tError Code: {:#04x}"
                 "\n\r\tError Register: {:#02x}"
                 "\n\r\tError Bit: {:#02x}"
                 "\n\r\tInfo Code: {:#08x}",
                 nodeId,
                 errorCode,
                 errorRegister,
                 errorBit,
                 infoCode);
}

void CanOpen::hbConsumerPreCallback(void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in hbConsumerPreCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
}

void CanOpen::hbConsumerNmtChangedCallback(uint8_t nodeId, uint8_t idx, CO_NMT_internalState_t nmtState, void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in hbConsumerNmtChangedCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_DEBUG(
      that->m_tag, "NMT state changed to {} on node {:#02x} (idx: {})", canOpenNmtStateToStr(nmtState), nodeId, idx);
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
    BR_LOG_DEBUG(that->m_port, "Remote node {:#02x} (idx: {}) reset!", nodeId, idx);
}

void CanOpen::nmtPreCallback(void* arg)
{
    BR_CORE_ASSERT(arg != nullptr, "Arg pointer null in nmtPreCallback");
    CanOpen* that = static_cast<CanOpen*>(arg);
    BR_LOG_DEBUG(that->m_tag, "NMT pre callback");
}

void CanOpen::nmtChangedCallback(CO_NMT_internalState_t state)
{
    BR_LOG_DEBUG(s_tag, "NMT state changed to {}", canOpenNmtStateToStr(state));
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
