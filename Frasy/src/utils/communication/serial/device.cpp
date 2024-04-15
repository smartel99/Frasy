/**
 * @file    device.cpp
 * @author  Samuel Martel
 * @date    2022-12-13
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
#include "device.h"

#include "utils/commands/built_in/id.h"
#include "utils/commands/built_in/status/reply.h"

#include <barrier>

namespace Frasy::Communication
{

SerialDevice::SerialDevice(DeviceInfo info, bool open)
: m_label(fmt::format("UART {}", info.ComPort)),
  m_device(info.ComPort,
           460800,
           serial::Timeout::simpleTimeout(10),
           serial::eightbits,
           serial::parity_none,
           serial::stopbits_one,
           serial::flowcontrol_software),
  m_info(info.Info)
{
    if (open) { this->open(); }
}

SerialDevice& SerialDevice::operator=(SerialDevice&& o) noexcept
{
    bool reopen = false;
    if (o.m_device.isOpen())
    {
        reopen = true;
        o.close();
    }

    m_label       = std::move(o.m_label);
    m_rxBuff      = std::move(o.m_rxBuff);
    m_pending     = std::move(o.m_pending);
    m_info        = std::move(o.m_info);
    m_commands    = std::move(o.m_commands);
    m_typeManager = std::move(o.m_typeManager);
    m_log         = o.m_log;
    m_ready       = o.m_ready;

    serial::Timeout timeout = serial::Timeout::simpleTimeout(10);
    m_device.setTimeout(timeout);
    m_device.setBaudrate(460800);
    m_device.setPort(o.m_device.getPort());
    if (reopen) { open(); }
    return *this;
}

ResponsePromise& SerialDevice::transmit(Packet pkt)
{
    // pkt MUST be a copy. We might modify it here in order to set a proper Transaction ID and compute its CRC
    // The user has the right to use the same commands as below in order to generate a packet that will match
    // our own modified packet, so they will not be "surprised"
    pkt.MakeTransactionId();
    pkt.ComputeCrc();

    BR_LOG_DEBUG(m_label, "Sending packet '{:08X}'", pkt.Header.TransactionId);
    std::vector<uint8_t> data = static_cast<std::vector<uint8_t>>(pkt);
    std::lock_guard promiseLock {m_promiseLock};
    m_device.write(data);
    m_device.flushOutput();
    auto&& [it, success] = m_pending.insert_or_assign(pkt.Header.TransactionId, std::move(ResponsePromise {}));
    return it->second;
}

[[nodiscard]] std::vector<trs_id_t> SerialDevice::getPendingTransactions()
{
    std::vector<trs_id_t> ids;
    ids.reserve(m_pending.size());
    std::lock_guard lock {m_promiseLock};
    for (auto&& [id, p] : m_pending) { ids.push_back(id); }

    return ids;
}

void SerialDevice::checkForPackets()
{
    // Check if we received a full packet somewhere.
    size_t sofPos = 0;
    do {
        sofPos = m_rxBuff.find(Packet::s_packetStartFlag);
        if (sofPos != std::string::npos)
        {
            // Found a start of packet! Flush the preceding data, it's probably not valid anymore...
            if (sofPos > 0) { m_rxBuff.erase(0, sofPos); }

            // Start of packet is now the first byte.
            sofPos = 0;

            // Find the end of the packet.
            size_t eofPos = m_rxBuff.find(Packet::s_packetEndFlag);
            if (eofPos != std::string::npos)
            {
                // We got a full packet!
                std::string raw = m_rxBuff.substr(0, eofPos + 1);
                m_rxBuff.erase(0, eofPos+1);
                try
                {
                    Packet packet = Packet {{raw.begin(), raw.end()}};

                    if (packet.Header.Modifiers.IsResponse)
                    {
                        try
                        {
                            std::lock_guard lock {m_promiseLock};
                            auto&           promise = m_pending.at(packet.Header.TransactionId);
                            promise.Promise.set_value(packet);
                            BR_LOG_DEBUG(m_label, "Received response for '{:08X}'", packet.Header.TransactionId);
                        }
                        catch (std::out_of_range&)
                        {
                            BR_LOG_ERROR(m_label,
                                         "Received response for packet '{:08X}', which doesn't exist!",
                                         packet.Header.TransactionId);
                        }
                        catch (std::future_error& e)
                        {
                            BR_LOG_ERROR(m_label, "In transaction '{:08X}': {}", packet.Header.TransactionId, e.what());
                        }
                    }
                    else
                    {
                        auto dispatcher = Commands::CommandManager::Get().MakeDispatcher(
                          Commands::CommandEvent {[this](const Packet& pkt) { transmit(pkt); }, this->m_label, packet});
                        dispatcher.Dispatch();
                    }
                }
                catch (BasePacketException& e)
                {
                    BR_LOG_ERROR(m_label, "A packet error occurred: {}", e.what());
                }
            }
        }
    } while (sofPos != std::string::npos);
}

void SerialDevice::open()
{
    // If already open, re-open.
    if (m_device.isOpen()) { close(); }

    try
    {
        m_device.open();
    }
    catch (std::exception& e)
    {
        BR_LOG_ERROR(m_label, "While opening '{}': {}", m_device.getPort(), e.what());
        return;
    }

    m_cleanerThread = std::thread([this] { cleanerTask(); });

    m_shouldRun = true;
    std::barrier rxReady {2};
    m_rxThread = std::thread(
      [&]()
      {
          BR_LOG_INFO(m_label, "Started RX listener on '{}'", m_device.getPort());
          std::string endOfPacket = std::string(1, Packet::s_packetEndFlag);
          rxReady.arrive_and_wait();
          while (m_shouldRun)
          {
              std::string read;
              try
              {
                  read = m_device.readline(Packet::s_maximumPacketSize, endOfPacket);
                  if (!read.empty()) { BR_LOG_TRACE(m_label, "RX: {}", read); }
                  m_rxBuff += read;
              }
              catch (std::exception& e)
              {
                  if (m_shouldRun) { BR_LOG_ERROR(m_label, "An error occurred in the listener thread: {}", e.what()); }
                  break;
              }
              if (!read.empty() && read.back() == Packet::s_packetEndFlag) { checkForPackets(); }
          }
          BR_LOG_INFO(m_label, "RX listener terminated on '{}'", m_device.getPort());
      });
    rxReady.arrive_and_wait();

    getCapabilities();
}

void SerialDevice::close()
{
    // If already closed, don't do anything.
    if (m_device.isOpen())
    {
        m_shouldRun = false;

        // Forcefully terminate *any* I/O operation done by the thread.
        if (CancelSynchronousIo(m_rxThread.native_handle()) == 0)
        {
            BR_LOG_WARN(m_label, "CancelSynchronousIo returned {}", GetLastError());
        }
        if (m_rxThread.joinable()) { m_rxThread.join(); }
        m_device.close();

        using namespace std::chrono_literals;
        while (!m_pending.empty()) { std::this_thread::sleep_for(10ms); }
        m_cleanerRun = false;
        if (m_cleanerThread.joinable()) { m_cleanerThread.join(); }
    }
}

void SerialDevice::reset()
{
    transmit(Packet::Request(Actions::CommandId::Reset)).OnTimeout([]() {}).Async();
}

bool SerialDevice::getLog() const
{
    return m_log;
}

void SerialDevice::setLog(bool enable)
{
    transmit(Packet::Request(Actions::CommandId::Log).MakePayload(enable))
      .OnComplete([&]([[maybe_unused]] const Packet& packet) { m_log = enable; })
      .Await();
}

void SerialDevice::getCapabilities()
{
    namespace fa = Frasy::Actions;
    transmit(Packet::Request(fa::CommandId::Identify))
      .OnComplete([&](const Packet& packet) { m_info = fa::Identify::Info(packet.FromPayload<fa::Identify::Reply>()); })
      .Await();

    if (m_ready)
    {
        BR_LOG_DEBUG(m_label, "Already loaded");
        return;
    }

    BR_LOG_DEBUG(m_label, "Getting capabilities");

    try
    {
        m_commands.clear();

        // TODO fix me
        //        Transmit(Packet::Request(CommandId::Log).MakePayload(true))
        //          .OnComplete(
        //            [&](const Packet& packet)
        //            {
        //                if (packet.Header.CommandId != static_cast<cmd_id_t>(CommandId::Status))
        //                {
        //                    throw std::exception("Invalid command");
        //                }
        //
        //                auto response = packet.FromPayload<Actions::Status::Reply>();
        //                if (response.Code != Actions::Status::ErrorCode::E::NoError)
        //                {
        //                    throw std::exception(response.Message.c_str());
        //                }
        //
        //                BR_LOG_DEBUG(m_label, "Logs enabled");
        //                m_log = true;
        //            })
        //          .Await();

        GetDeviceCommands();
        GetDeviceEnums();
        GetDeviceStructs();

        m_ready = true;

        BR_LOG_INFO(m_label,
                    "loaded: {} commands, {} structs, {} enums",
                    m_commands.size(),
                    m_typeManager.GetStructs().size(),
                    m_typeManager.GetEnums().size());
    }
    catch (std::exception& e)
    {
        BR_LOG_ERROR(m_label, "Failed to get board capabilities: {}", e.what());
    }
}

void SerialDevice::GetDeviceStructs()
{
    auto structIds = transmit(Packet::Request(Actions::CommandId::StructsList)).Collect<std::vector<Type::BasicInfo>>();
    for (const auto& info : structIds)
    {
        BR_LOG_DEBUG(m_label, "Getting information for '{}'...", info.name);
        for (size_t attempt = 0; attempt < s_maxAttempts; attempt++)
        {
            Packet resp = transmit(Packet::Request(Actions::CommandId::StructInfo).MakePayload(info.id)).Collect();
            if (resp.Header.CommandId == static_cast<cmd_id_t>(Actions::CommandId::StructInfo))
            {
                m_typeManager.AddStruct(info.id, resp.FromPayload<Type::Struct>());
                break;
            }
            if (resp.Header.CommandId == static_cast<cmd_id_t>(Actions::CommandId::Status))
            {
                auto [message, code] = resp.FromPayload<Actions::Status::Reply>();
                BR_LOG_ERROR(m_label, "Device returned an error: ({}) {}", static_cast<size_t>(code), message);
            }
            else { BR_LOG_ERROR(m_label, "Invalid command from device"); }
        }
    }
}

void SerialDevice::GetDeviceEnums()
{
    auto enumIds = transmit(Packet::Request(Actions::CommandId::EnumsList)).Collect<std::vector<Type::BasicInfo>>();
    for (const auto& info : enumIds)
    {
        BR_LOG_DEBUG(m_label, "Getting information for '{}'...", info.name);
        for (size_t attempt = 0; attempt < s_maxAttempts; attempt++)
        {
            Packet resp = transmit(Packet::Request(Actions::CommandId::EnumInfo).MakePayload(info.id)).Collect();
            if (resp.Header.CommandId == static_cast<cmd_id_t>(Actions::CommandId::EnumInfo))
            {
                m_typeManager.AddEnum(info.id, resp.FromPayload<Type::Enum>());
                break;
            }
            if (resp.Header.CommandId == static_cast<cmd_id_t>(Actions::CommandId::Status))
            {
                auto [message, code] = resp.FromPayload<Actions::Status::Reply>();
                BR_LOG_ERROR(m_label, "Device returned an error: ({}) {}", static_cast<size_t>(code), message);
            }
            else { BR_LOG_ERROR(m_label, "Invalid command from device"); }
        }
    }
}

void SerialDevice::GetDeviceCommands()
{
    auto cmdNames = transmit(Packet::Request(Actions::CommandId::CommandsList)).Collect<std::vector<std::string>>();

    for (const auto& name : cmdNames)
    {
        BR_LOG_DEBUG(m_label, "Getting information for '{}'...", name);
        for (size_t attempt = 0; attempt < s_maxAttempts; attempt++)
        {
            Packet resp = transmit(Packet::Request(Actions::CommandId::CommandInfo).MakePayload(name)).Collect();
            if (resp.Header.CommandId == static_cast<cmd_id_t>(Actions::CommandId::CommandInfo))
            {
                auto info           = resp.FromPayload<Actions::CommandInfo::Reply>();
                m_commands[info.Id] = info;
                break;
            }
            if (resp.Header.CommandId == static_cast<cmd_id_t>(Actions::CommandId::Status))
            {
                auto [message, code] = resp.FromPayload<Actions::Status::Reply>();
                BR_LOG_ERROR(m_label, "Device returned an error: ({}) {}", static_cast<size_t>(code), message);
            }
            else { BR_LOG_ERROR(m_label, "Invalid command from device"); }
        }
    }
}

void SerialDevice::cleanerTask()
{
    m_cleanerRun = true;
    while (m_cleanerRun)
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
        std::vector<uint32_t> consumed;
        std::lock_guard       lock {m_promiseLock};
        consumed.reserve(m_pending.size());
        for (const auto& [id, response] : m_pending)
        {
            if (response.IsConsumed()) { consumed.push_back(id); }
        }
        for (const auto& id : consumed) { m_pending.erase(id); }
    }
}
}    // namespace Frasy::Communication
