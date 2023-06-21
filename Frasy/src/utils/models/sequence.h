/**
 * @file    sequence.h
 * @author  Paul Thomas
 * @date    5/2/2023
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
#ifndef KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_SEQUENCE_H
#define KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_SEQUENCE_H

#include "execution_state.h"
#include "test.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace Frasy::Models
{

struct Sequence
{
    bool                                  enabled = false;
    std::vector<ExecutionState>           state;
    std::unordered_map<std::string, Test> tests = {};

    Sequence()                      = default;
    Sequence(const Sequence& other) = default;
    explicit Sequence(bool enabled, const std::vector<std::string>& tests) : enabled(enabled)
    {
        for (const auto& test : tests) { this->tests[test] = Test {true}; }
    }
};

}    // namespace Frasy::Models

#endif    // KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_SEQUENCE_H
