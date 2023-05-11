/**
 * @file    solution.cpp
 * @author  Paul Thomas
 * @date    5/7/2023
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

#include "solution.h"

#include "Brigerad/Core/Log.h"
#include "json.hpp"

#include <fstream>

namespace Frasy::Models
{

void Solution::Clear()
{
    sections.clear();
    sequences.clear();
}

void Solution::SetSequenceEnable(const std::string& sequence, bool enabled)
{
    sequences.at(sequence).enabled = enabled;
}

void Solution::SetTestEnable(const std::string& sequence, const std::string& test, bool enabled)
{
    sequences.at(sequence).tests.at(test).enabled = enabled;
}
}    // namespace Frasy::Models