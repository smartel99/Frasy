/**
 * @file    scope.h
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
#ifndef KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_SCOPE_H
#define KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_SCOPE_H

#include <string>

struct Scope
{
    enum Result
    {
        unknown,
        success,
        skipped,
        error,
    };

    Result      result = unknown;
    std::string sequence;
    std::string test;

    Scope() = default;
    Scope(std::string sequence) : sequence(sequence) {}
    Scope(std::string sequence, std::string test) : sequence(sequence), test(test) {}


    bool IsSequence() { return !sequence.empty() && test.empty(); }
    bool IsTest() { return !sequence.empty() && !test.empty(); }
};

#endif    // KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_SCOPE_H
