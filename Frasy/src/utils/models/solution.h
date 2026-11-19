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

#include <istream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Frasy::Models {
struct Solution {
    using subsequence = std::pair<std::string, std::vector<std::vector<std::string>>>;
    using section     = std::vector<std::vector<subsequence>>;

    std::vector<section>                      sections      = {};
    std::unordered_map<std::string, Sequence> sequences     = {};
    std::vector<std::string>                  sequenceOrder = {};    //!< Execution order of `sequences`.

    void Clear();

    /**
     * Build from the generation output (`lua/solution.json`).
     * Entries are enabled, states are empty. Throws on invalid or malformed input.
     */
    [[nodiscard]] static Solution FromGeneration(std::istream& generation);

    /**
     * Replace with `fresh`, keeping enable flags of entries that still exist, states reset for `uutCount`.
     * Left untouched if it throws.
     */
    void ReplaceWith(Solution&& fresh, std::size_t uutCount);

    /// Add a sequence if missing, keeping first-appearance order.
    Sequence& AddSequence(const std::string& sequence);
    /// Add a test (and its sequence) if missing, keeping first-appearance order.
    void AddTest(const std::string& sequence, const std::string& test);

    /// Copy enable flags of sequences and tests that also exist in `other`.
    void CopyEnableStates(const Solution& other);
    /// Size every state vector to `uutCount + 1` (UUTs are 1-based), set to idle or disabled.
    void ResetStates(std::size_t uutCount);

    /**
     * Set a sequence's state for a UUT. `running`, `passed`, `failed` and `error` never overwrite `disabled` or
     * `skipped` (Lua reports the start of those, and the end too for tests). `disabled` and `skipped` always apply,
     * and also to the sequence's tests that are still idle.
     * @return true if the state changed, false if unchanged, refused or the sequence or UUT is unknown
     */
    bool SetSequenceState(std::size_t uut, const std::string& sequence, ExecutionState state);
    /// Same as SetSequenceState(), for a test.
    bool SetTestState(std::size_t uut, const std::string& sequence, const std::string& test, ExecutionState state);

    /**
     * Apply an event of Lua's `__progress.report(type, uut, name, parentA, parentB, pass)`.
     * `name` is the sequence for sequence events, the test for test events (`parent` is then its sequence).
     * Expectations and unknown types are ignored.
     * @return true if a state changed
     */
    bool ApplyProgress(
      std::string_view type, std::size_t uut, const std::string& name, const std::string& parent, bool pass);

    /// Mark every sequence and test of a UUT as disabled.
    void DisableUut(std::size_t uut);
    /// End of a run: whatever is still running was aborted, it becomes `error`.
    void FinishRun();

    void SetSequenceEnable(const std::string& sequence, bool enabled);
    void SetTestEnable(const std::string& sequence, const std::string& test, bool enabled);
};
}    // namespace Frasy::Models

#endif    // KONGSBERG_FRASY_FRASY_SRC_UTILS_MODELS_SOLUTION_H
