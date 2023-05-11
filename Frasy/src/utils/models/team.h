/**
 * @file    team.h
 * @author  Paul Thomas
 * @date    5/4/2023
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
#ifndef KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_TEAM_H
#define KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_TEAM_H
#include "uut.h"

#include <vector>

struct Team
{
    std::vector<Uut*> uuts = {};
    std::string       name = "";
};

#endif    // KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_TEAM_H
