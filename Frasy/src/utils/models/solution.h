/**
 * @file    solution.h
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
#ifndef KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_SOLUTION_H
#define KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_SOLUTION_H
#include "sequence.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Frasy::Models
{
struct Solution
{
    using subsequence = std::pair<std::string, std::vector<std::vector<std::string>>>;
    using section     = std::vector<std::vector<subsequence>>;

    std::vector<section>                      sections  = {};
    std::unordered_map<std::string, Sequence> sequences = {};

    void Clear();

    void SetSequenceEnable(const std::string& sequence, bool enabled);
    void SetTestEnable(const std::string& sequence, const std::string& test, bool enabled);
};
}    // namespace Frasy::Models

#endif    // KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_SOLUTION_H
