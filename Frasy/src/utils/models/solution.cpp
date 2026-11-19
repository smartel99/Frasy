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
 * not, see <https://www.gnu.org/licenses/>.
 */

#include "solution.h"

#include "Brigerad/Core/Log.h"
#include "json.hpp"

#include <fstream>

namespace Frasy::Models {

void Solution::Clear()
{
    sections.clear();
    sequences.clear();
    sequenceOrder.clear();
}

Solution Solution::FromGeneration(std::istream& generation)
{
    // Solution
    // - Section
    // - - Section stage
    // - - - Sequence (Actually a subsequence)
    // - - - - Sequence name
    // - - - - Sequence stage
    // - - - - - Test
    const auto gSolution = nlohmann::json::parse(generation);

    Solution solution;
    solution.sections.reserve(gSolution.size());
    for (const auto& gSc : gSolution) {
        auto& sc = solution.sections.emplace_back();
        sc.reserve(gSc.size());
        for (const auto& gScS : gSc) {
            auto& scs = sc.emplace_back();
            scs.reserve(gScS.size());
            for (const auto& gSq : gScS) {
                auto& sq = scs.emplace_back();
                sq.first = gSq.at("name").get<std::string>();
                solution.AddSequence(sq.first);
                const auto& gTests = gSq.at("tests");
                sq.second.reserve(gTests.size());
                for (const auto& gSqS : gTests) {
                    auto& sqs = sq.second.emplace_back();
                    sqs.reserve(gSqS.size());
                    for (const auto& gT : gSqS) {
                        solution.AddTest(sq.first, sqs.emplace_back(gT.get<std::string>()));
                    }
                }
            }
        }
    }
    return solution;
}

void Solution::ReplaceWith(Solution&& fresh, std::size_t uutCount)
{
    // Everything that can throw runs on `fresh`, so `*this` is untouched on failure.
    fresh.CopyEnableStates(*this);
    fresh.ResetStates(uutCount);
    *this = std::move(fresh);
}

Sequence& Solution::AddSequence(const std::string& sequence)
{
    auto [it, inserted] = sequences.try_emplace(sequence);
    if (inserted) { sequenceOrder.push_back(sequence); }
    return it->second;
}

void Solution::AddTest(const std::string& sequence, const std::string& test)
{
    Sequence& sq = AddSequence(sequence);
    if (sq.tests.try_emplace(test).second) { sq.testOrder.push_back(test); }
}

void Solution::CopyEnableStates(const Solution& other)
{
    for (auto& [sName, sequence] : sequences) {
        auto oSq = other.sequences.find(sName);
        if (oSq == other.sequences.end()) { continue; }
        sequence.enabled = oSq->second.enabled;
        for (auto& [tName, test] : sequence.tests) {
            auto oT = oSq->second.tests.find(tName);
            if (oT != oSq->second.tests.end()) { test.enabled = oT->second.enabled; }
        }
    }
}

void Solution::ResetStates(std::size_t uutCount)
{
    for (auto& [sName, sequence] : sequences) {
        sequence.state.assign(uutCount + 1, sequence.enabled ? idle : disabled);
        for (auto& [tName, test] : sequence.tests) {
            test.state.assign(uutCount + 1, sequence.enabled && test.enabled ? idle : disabled);
        }
    }
}

void Solution::SetSequenceEnable(const std::string& sequence, bool enabled)
{ sequences.at(sequence).enabled = enabled; }

void Solution::SetTestEnable(const std::string& sequence, const std::string& test, bool enabled)
{ sequences.at(sequence).tests.at(test).enabled = enabled; }
}    // namespace Frasy::Models