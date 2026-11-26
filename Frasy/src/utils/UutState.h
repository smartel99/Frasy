/**
 * @file    UutState.h
 * @author  Paul Thomas
 * @date    3/27/2023
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
#ifndef COPY_LUA_PY_FRASY_SRC_UTILS_LUA_UUTSTATE_H
#define COPY_LUA_PY_FRASY_SRC_UTILS_LUA_UUTSTATE_H

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace Frasy {

enum class UutState {
    Disabled,
    Idle,
    Waiting,
    Running,
    Passed,
    Failed,
    Error,
    IncompletePassed,    //!< Something was disabled; everything that ran passed. Not a pass.
    IncompleteFailed,    //!< Something was disabled, and something that ran failed.
};

/** Lowercase name of a state, as reported by the MCP interface. */
[[nodiscard]] constexpr std::string_view toString(UutState state) noexcept
{
    switch (state) {
        case UutState::Disabled: return "disabled";
        case UutState::Idle: return "idle";
        case UutState::Waiting: return "waiting";
        case UutState::Running: return "running";
        case UutState::Passed: return "passed";
        case UutState::Failed: return "failed";
        case UutState::Error: return "error";
        case UutState::IncompletePassed: return "incomplete_passed";
        case UutState::IncompleteFailed: return "incomplete_failed";
    }
    return "unknown";
}

/** Verdict of a UUT or of a whole run, from the least to the most severe. */
enum class RunVerdict : uint8_t {
    Passed,
    Incomplete,
    Failed,
    Error,
};

/**
 * @brief Verdict of one UUT.
 * @return The verdict, or nothing for states that aren't a result (disabled, idle, waiting, running).
 *         An incomplete UUT where something failed is a failure.
 */
[[nodiscard]] constexpr std::optional<RunVerdict> getVerdict(UutState state) noexcept
{
    switch (state) {
        case UutState::Passed: return RunVerdict::Passed;
        case UutState::IncompletePassed: return RunVerdict::Incomplete;
        case UutState::Failed:
        case UutState::IncompleteFailed: return RunVerdict::Failed;
        case UutState::Error: return RunVerdict::Error;
        case UutState::Disabled:
        case UutState::Idle:
        case UutState::Waiting:
        case UutState::Running: break;
    }
    return std::nullopt;
}

/**
 * @brief Verdict of a run: the most severe verdict of its UUTs.
 * @return The verdict; Passed when no UUT has a result.
 */
[[nodiscard]] constexpr RunVerdict getRunVerdict(std::span<const UutState> states) noexcept
{
    auto verdict = RunVerdict::Passed;
    for (const auto state : states) {
        if (const auto uutVerdict = getVerdict(state)) { verdict = std::max(verdict, *uutVerdict); }
    }
    return verdict;
}

/** Lowercase name of a verdict, as reported by the MCP interface. */
[[nodiscard]] constexpr std::string_view toString(RunVerdict verdict) noexcept
{
    switch (verdict) {
        case RunVerdict::Passed: return "passed";
        case RunVerdict::Incomplete: return "incomplete";
        case RunVerdict::Failed: return "failed";
        case RunVerdict::Error: return "error";
    }
    return "unknown";
}

}    // namespace Frasy
#endif    // COPY_LUA_PY_FRASY_SRC_UTILS_LUA_UUTSTATE_H
