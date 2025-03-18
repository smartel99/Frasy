/**
 * @file    solution_loader.h
 * @author  Paul Thomas
 * @date    3/17/2025
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

#ifndef SOLUTION_LOADER_H
#define SOLUTION_LOADER_H

#include <string>
#include <vector>
#include <json.hpp>

namespace Frasy::Report::SolutionLoader
{
    using Test = std::string;
    using Sequence = std::pair<std::string, std::vector<Test>>;
    using Solution = std::vector<Sequence>;

    nlohmann::json loadJson(const std::string& path);
    Solution loadSolution();
}

#endif //SOLUTION_LOADER_H
