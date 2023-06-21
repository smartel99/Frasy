/**
 * @file    enumerator.h
 * @author  Samuel Martel
 * @date    2022-12-14
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

#ifndef FRASY_UTILS_COMMUNICATION_SERIAL_ENUMERATOR_H
#define FRASY_UTILS_COMMUNICATION_SERIAL_ENUMERATOR_H

#include "utils/commands/built_in/identify/reply.h"

#include <string>
#include <vector>

namespace Frasy::Communication
{
struct DeviceInfo
{
    std::string             ComPort;
    Actions::Identify::Info Info;
};

std::vector<DeviceInfo> EnumerateInstrumentationCards();
}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMMUNICATION_SERIAL_ENUMERATOR_H
