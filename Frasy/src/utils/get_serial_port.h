/**
 * @file    get_serial_port.h
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

#ifndef FRASY_UTILS_GET_SERIAL_PORT_WINDOWS_H
#define FRASY_UTILS_GET_SERIAL_PORT_WINDOWS_H

#include "Brigerad/Events/usb_event.h"
#include "Brigerad/Utils/Serial.h"

#include <string>
#include <vector>

std::string getSerialPort(const Brigerad::UsbEvent& event, const std::vector<serial::PortInfo>& ports);

#endif    // FRASY_UTILS_GET_SERIAL_PORT_WINDOWS_H
