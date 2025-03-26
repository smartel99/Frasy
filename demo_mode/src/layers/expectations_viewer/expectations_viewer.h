/**
 * @file    expectations_viewer.h
 * @author  Paul Thomas
 * @date    3/25/2025
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

#ifndef EXPECTATIONS_VIEWER_H
#define EXPECTATIONS_VIEWER_H

#include <utils/lua/expectation.h>
#include <vector>

void renderExpectations(const std::vector<Frasy::Lua::Expectation>& uut);

#endif    // EXPECTATIONS_VIEWER_H
