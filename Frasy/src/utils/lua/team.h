/**
 * @file    team.h
 * @author  Paul Thomas
 * @date    3/17/2023
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
#ifndef FRASYLUA_TEAM_H
#define FRASYLUA_TEAM_H

#include <array>
#include <barrier>
#include <mutex>
#include <sol/sol.hpp>
#include <vector>

namespace Frasy::Lua {
class Team {
private:
    enum SyncState { pass, fail, critical_failure };
    std::size_t                                    m_teamSize = 0;
    std::unique_ptr<sol::state>                    m_teamState;
    std::array<std::unique_ptr<std::barrier<>>, 3> m_bShare;
    std::array<std::unique_ptr<std::barrier<>>, 2> m_bSync;
    std::unique_ptr<std::barrier<>>                m_wBarrier;
    std::size_t                                    m_wCount;
    std::size_t                                    m_wError;
    std::unique_ptr<std::mutex>                    m_mutex;
    std::vector<uint8_t>                           m_buf;
    std::vector<SyncState>                         m_syncStates;

public:
             Team() = default;
    explicit Team(std::size_t teamSize);
    void     InitializeState(sol::state_view other, std::size_t uut, std::size_t position, bool is_leader);
    void     Store(sol::state_view lua, const sol::object& o);
    std::optional<sol::object> Load(sol::state_view lua);

private:
    template<typename T>
    void Serialize(const T& t);
    template<typename T>
    T Deserialize(std::size_t& cur);

    void                       _Store(sol::state_view lua, const sol::object& o);
    std::optional<sol::object> _Load(sol::state_view lua, std::size_t& cur);
};
}    // namespace Frasy::Lua

#endif    // FRASYLUA_TEAM_H
