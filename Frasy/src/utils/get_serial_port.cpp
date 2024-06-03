/**
 * @file    get_serial_port.cpp
 * @author  Paul Thomas
 * @date    6/3/2024
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version. This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details. You should have received a copy of the GNU General Public License
 * along with this program. If not, see <a
 * href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#include "get_serial_port.h"

#include <regex>

std::string getSerialPort(const Brigerad::UsbEvent& event, const std::vector<serial::PortInfo>& ports)
{
    std::regex pVid("VID_[A-F0-9]+");
    std::regex pPid("PID_[A-F0-9]+");
    std::regex pMi("MI_[A-F0-9]+");

    std::string target = wstring_to_utf8(event.name);
    std::smatch match;

    if (!std::regex_search(target, match, pVid)) { return ""; }
    std::string tVid = match[0];
    if (!std::regex_search(target, match, pPid)) { return ""; }
    std::string tPid = match[0];
    std::string tMi  = "";
    if (std::regex_search(target, match, pMi)) { tMi = match[1]; }

    for (auto& port : ports) {
        std::regex oVid(tVid);
        std::regex oPid(tPid);
        if (!std::regex_search(port.hardware_id, match, oVid)) { continue; }
        if (!std::regex_search(port.hardware_id, match, oPid)) { continue; }
        if (!tMi.empty()) {
            std::regex oMi(tMi);
            if (!std::regex_search(port.hardware_id, match, oMi)) { continue; }
        }
        return port.port;
    }

    // Failed to find device
    return "";
}
