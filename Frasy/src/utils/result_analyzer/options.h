/**
 * @file    options.h
 * @author  Samuel Martel
 * @date    2023-05-02
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

#ifndef FRASY_SRC_UTILS_RESULT_ANALYZER_OPTIONS_H
#define FRASY_SRC_UTILS_RESULT_ANALYZER_OPTIONS_H

#include <array>
#include <vector>

namespace Frasy::Analyzers
{
struct ResultOptions
{
    std::vector<std::array<char, 32>> SerialNumbers = {};    //!< Serial numbers to analyze.
    std::vector<std::array<char, 32>> Uuts          = {};    //!< Uuts to analyze.
    std::vector<std::array<char, 32>> Sequences     = {};    //!< Sequences to analyze.
    std::vector<std::array<char, 32>> Tests         = {};    //!< Tests to analyze.

    bool Ganged = true;                                      //!< Combine the results of all UUT locations together.
};
}    // namespace Frasy::Analyzers
#endif    // FRASY_SRC_UTILS_RESULT_ANALYZER_OPTIONS_H
