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

void SerialDevice::CheckForPackets()
{
    // Check if we received a full packet somewhere.
    size_t sofPos;
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
                std::string raw = m_rxBuff.substr(0, eofPos);
                m_rxBuff.erase(0, eofPos);
                try
                {
                    Packet packet = Packet {{raw.begin(), raw.end()}};

                    if (packet.Header.Modifiers.IsResponse)
                    {
                        try
                        {
                            std::lock_guard lock {m_lock};
                            auto&           promise = m_pending.at(packet.Header.TransactionId);
                            promise.Promise.set_value(packet);
                            m_pending.erase(packet.Header.TransactionId);
                        }
                        catch (std::out_of_range&)
                        {
                            BR_LOG_ERROR(m_label,
                                         "Received response for packet '{:08X}', which doesn't exist!",
                                         packet.Header.TransactionId);
                        }
                    }
                    else
                    {
                        auto dispatcher = Commands::CommandManager::Get().MakeDispatcher(
                          Commands::CommandEvent {[this](const Packet& pkt) { Transmit(pkt); }, this->m_label, packet});
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

void SerialDevice::Open()
{
    // If already open, re-open.
    if (m_device.isOpen()) { Close(); }

    try
    {
        m_device.open();
    }
    catch (std::exception& e)
    {
        BR_LOG_ERROR(m_label, "While opening '{}': {}", m_device.getPort(), e.what());
        return;
    }

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
                  //                  if (!read.empty()) { BR_LOG_DEBUG(m_label, "RX: {}", read); }
                  m_rxBuff += read;
              }
              catch (std::exception& e)
              {
                  BR_LOG_ERROR(m_label, "An error occurred in the listener thread: {}", e.what());
                  break;
              }
              if (!read.empty() && read.back() == Packet::s_packetEndFlag) { CheckForPackets(); }
          }
          BR_LOG_INFO(m_label, "RX listener terminated on '{}'", m_device.getPort());
      });
    rxReady.arrive_and_wait();

    GetCapabilities();
}

void SerialDevice::Close()
{
    // If already closed, don't do anything.
    if (m_device.isOpen())
    {
        m_shouldRun = false;

        // Forcefully terminate *any* I/O operation done by the thread.
        if (CancelSynchronousIo(m_rxThread.native_handle()) != 0)
        {
            BR_LOG_WARN(m_label, "CancelSynchronousIo returned non-0");
        }
        if (m_rxThread.joinable()) { m_rxThread.join(); }
        m_device.close();
    }
}

void SerialDevice::Reset()
{
    Transmit(Packet::Request(Actions::CommandId::Reset)).OnTimeout([]() {}).Async();
}

bool SerialDevice::GetLog() const
{
    return m_log;
}

void SerialDevice::SetLog(bool enable)
{
    Transmit(Packet::Request(Actions::CommandId::Log).MakePayload(enable))
      .OnComplete([&](const Packet& packet) { m_log = enable; })
      .Await();
}

void SerialDevice::GetCapabilities()
{

    using namespace Frasy::Actions;
    Transmit(Packet::Request(CommandId::Identify))
      .OnComplete([&](const Packet& packet) { m_info = Identify::Info(packet.FromPayload<Identify::Reply>()); })
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
        m_structs.clear();
        m_enums.clear();

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

        auto cmdNames = Transmit(Packet::Request(CommandId::CommandsList)).Collect<std::vector<std::string>>();

        for (const auto& name : cmdNames)
        {
            Transmit(Packet::Request(CommandId::CommandInfo).MakePayload(name))
              .OnComplete(
                [&](const Packet& packet)
                {
                    if (packet.Header.CommandId == static_cast<cmd_id_t>(CommandId::CommandInfo))
                    {
                        m_commands.push_back(packet.FromPayload<Actions::CommandInfo::Reply>());
                        return;
                    }
                    if (packet.Header.CommandId == static_cast<cmd_id_t>(CommandId::Status))
                    {
                        throw std::exception(packet.FromPayload<std::string>().c_str());
                    }
                    throw std::exception("Invalid command");
                })
              .Await();
        }

        auto enumIds = Transmit(Packet::Request(CommandId::EnumsList)).Collect<std::vector<Type::BasicInfo>>();

        auto structIds = Transmit(Packet::Request(CommandId::StructsList)).Collect<std::vector<Type::BasicInfo>>();

        for (const auto& info : enumIds)
        {
            Transmit(Packet::Request(CommandId::EnumInfo).MakePayload(info.id))
              .OnComplete(
                [&](const Packet& packet)
                {
                    if (packet.Header.CommandId == static_cast<cmd_id_t>(CommandId::EnumInfo))
                    {
                        m_enums.push_back(packet.FromPayload<Type::Enum>());
                        return;
                    }
                    if (packet.Header.CommandId == static_cast<cmd_id_t>(CommandId::Status))
                    {
                        throw std::exception(packet.FromPayload<std::string>().c_str());
                    }
                    throw std::exception("Invalid command");
                })
              .Await();
        }

        for (const auto& info : structIds)
        {
            Transmit(Packet::Request(CommandId::StructInfo).MakePayload(info.id))
              .OnComplete(
                [&](const Packet& packet)
                {
                    if (packet.Header.CommandId == static_cast<cmd_id_t>(CommandId::StructInfo))
                    {
                        m_structs.push_back(packet.FromPayload<Type::Struct>());
                        return;
                    }
                    if (packet.Header.CommandId == static_cast<cmd_id_t>(CommandId::Status))
                    {
                        throw std::exception(packet.FromPayload<std::string>().c_str());
                    }
                    throw std::exception("Invalid command");
                })
              .Await();
        }

        m_ready = true;

        BR_LOG_INFO(
          m_label, "loaded: {} commands, {} structs, {} enums", m_commands.size(), m_structs.size(), m_enums.size());
    }
    catch (std::exception& e)
    {
        BR_LOG_ERROR(m_label, "Failed to get board capabilities: {}", e.what());
    }
}
}    // namespace Frasy::Communication
