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

#include <Windows.h>

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
            m_rxBuff.erase(0, sofPos - 1);

            // Start of packet is now the first byte.
            sofPos = 0;

            // Find the end of the packet.
            size_t eofPos = m_rxBuff.find(Packet::s_packetEndFlag);
            if (eofPos != std::string::npos)
            {
                // We got a full packet!
                std::string raw = m_rxBuff.substr(0, eofPos);
                m_rxBuff.erase(0, eofPos);
                BR_LOG_DEBUG(m_label, "Received Packet: {:02X}", fmt::join(raw, ", "));
                try
                {
                    Packet packet = Packet {{raw.begin(), raw.end()}};

                    if (packet.Header.Modifiers.IsResponse)
                    {
                        try
                        {
                            std::lock_guard lock(m_lock);
                            auto&           promise = m_pending.at(packet.Header.PacketId);
                            promise.Promise.set_value(packet);
                            m_pending.erase(packet.Header.PacketId);
                        }
                        catch (std::out_of_range&)
                        {
                            BR_LOG_ERROR(m_label,
                                         "Received response for packet '{:08X}', which doesn't exist!",
                                         packet.Header.PacketId);
                        }
                    }
                    else
                    {
                        auto dispatcher = Commands::CommandManager::Get().MakeDispatcher(
                          Commands::CommandEvent {[this](const Packet& pkt) { Transmit(pkt); }, packet});
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

void SerialDevice::Close()
{
    // If already closed, don't do anything.
    if (m_device.isOpen())
    {
        m_shouldRun = false;

        // Forcefully terminate *any* I/O operation done by the thread.
        if (CancelSynchronousIo(m_rxThread.native_handle()))
        {
            BR_LOG_WARN(m_label, "CancelSynchronousIo returned non-0");
        }
        if (m_rxThread.joinable()) { m_rxThread.join(); }
        m_device.close();
    }
}

void SerialDevice::EnumerateDeviceCapabilities()
{
    // Get Command ID from index
    // Get Command Name
    // Get Command Help
    // Get Command Argument count
    // For each argument:
    //  Get Argument Name
    //  Get Argument Help
    //  Get Argument Type
    //  Get Number of values (if > 1, argument is an array)
    //  Get Minimum value
    //  Get Maximum value
    // Get Command Returned value count
    // For each returned value:
    //  Get Returned value name
    //  Get returned value help
    //  Get returned value type
}
}    // namespace Frasy::Communication
