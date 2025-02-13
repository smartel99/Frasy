/**
 * @file    team.cpp
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
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "team.h"

#include "utils.h"

#include <array>
#include <bit>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std::literals::chrono_literals;

namespace Frasy::Lua {
Team::Team(std::size_t teamSize)
{
    m_teamState = std::make_unique<sol::state>();
    m_teamSize  = teamSize;
    std::ranges::for_each(m_bShare, [&](auto& item) { item = std::make_unique<std::barrier<>>(m_teamSize); });
    std::ranges::for_each(m_bSync, [&](auto& item) { item = std::make_unique<std::barrier<>>(m_teamSize); });
    m_wBarrier = std::make_unique<std::barrier<>>(m_teamSize);
    m_wCount   = 0;
    m_wError   = 0;
    m_mutex    = std::make_unique<std::mutex>();
    m_syncStates.resize(m_teamSize);
}

void Team::InitializeState(sol::state_view other, std::size_t uut, std::size_t position, bool is_leader)
{
    other["Team"]["__tell"] = [&](const sol::object& value) {
        m_bShare[0]->arrive_and_wait();
        m_teamState->set("share", copy(value, *m_teamState));
        std::atomic_thread_fence(std::memory_order_release);
        m_bShare[1]->arrive_and_wait();
        m_bShare[2]->arrive_and_wait();
        std::atomic_thread_fence(std::memory_order_acquire);
        m_teamState->set("share", sol::nil);
        std::atomic_thread_fence(std::memory_order_release);
    };

    other["Team"]["__get"] = [&, other]() -> std::optional<sol::object> {
        m_bShare[0]->arrive_and_wait();
        m_bShare[1]->arrive_and_wait();
        m_mutex->lock();
        std::atomic_thread_fence(std::memory_order_acquire);
        auto value = copy(m_teamState->get<sol::object>("share"), other);
        std::atomic_thread_fence(std::memory_order_release);
        m_mutex->unlock();
        m_bShare[2]->arrive_and_wait();
        return value;
    };

    other["Team"]["__wait"] = [&](sol::function routine) {
        m_mutex->lock();
        m_wCount++;
        m_mutex->unlock();

        bool team_is_ready = false;
        while (!team_is_ready) {
            m_mutex->lock();
            team_is_ready = m_wCount + m_wError == m_teamSize;
            m_mutex->unlock();
            routine();
        }

        m_mutex->lock();
        m_wCount = 0;
        std::ranges::for_each(m_bShare,
                              [&](auto& item) { item = std::make_unique<std::barrier<>>(m_teamSize - m_wError); });
        m_mutex->unlock();
        m_wBarrier->arrive_and_wait();
    };

    other["Team"]["__done"] = [&]() {
        m_mutex->lock();
        m_wCount++;
        m_mutex->unlock();
        m_bShare[0]->arrive_and_drop();
        m_bShare[1]->arrive_and_drop();
        m_bShare[2]->arrive_and_drop();
        m_wBarrier->arrive_and_wait();
    };

    other["Team"]["__sync"] = [&, position, is_leader](int status) {
        m_mutex->lock();
        m_syncStates[position - 1] = static_cast<SyncState>(status);
        m_mutex->unlock();
        m_bSync[0]->arrive_and_wait();
        if (is_leader) {
            std::ranges::for_each(m_bShare, [&](auto& item) { item = std::make_unique<std::barrier<>>(m_teamSize); });
            m_wBarrier = std::make_unique<std::barrier<>>(m_teamSize);
            m_wCount   = 0;
            m_wError   = 0;
        }
        m_bSync[1]->arrive_and_wait();
        if (std::ranges::any_of(m_syncStates, [](SyncState state) { return state == critical_failure; })) {
            return critical_failure;
        }
        if (std::ranges::any_of(m_syncStates, [](SyncState state) { return state == fail; })) {
            return fail;
        }
        return pass;
    };

    other["Team"]["__fail"] = [&] {
        m_mutex->lock();
        m_wError++;
        m_mutex->unlock();
        std::ranges::for_each(m_bShare, [](auto& b) { b->arrive_and_drop(); });
        m_wBarrier->arrive_and_drop();
    };
}

template<typename T>
void Team::Serialize(const T& t)
{
    if constexpr (std::is_same_v<T, std::string>) {
        auto size = std::bit_cast<std::array<uint8_t, sizeof(std::size_t)>>(t.size());
        m_buf.insert(m_buf.end(), size.begin(), size.end());
        m_buf.insert(m_buf.end(), t.begin(), t.end());
    }
    else if constexpr (std::is_arithmetic_v<T>) {
        auto vb = std::bit_cast<std::array<uint8_t, sizeof(T)>>(t);
        m_buf.insert(m_buf.end(), vb.begin(), vb.end());
    }
    else if constexpr (std::is_enum_v<T>) {
        Serialize(static_cast<std::underlying_type_t<T>>(t));
    }
}

template<typename T>
T Team::Deserialize(std::size_t& cur)
{
    std::size_t i = cur;
    if constexpr (std::is_same_v<T, std::string>) {
        auto size = std::bit_cast<std::size_t>(m_buf.data() + i);
        cur += 1 + size;
        return std::string {m_buf.data() + i + 1, m_buf.data() + i + 1 + size};
    }
    else if constexpr (std::is_integral_v<T> && std::negation_v<std::is_same<bool, T>>) {
        cur += sizeof(T);
        return *std::bit_cast<T*>(m_buf.data() + i);
    }
    else if constexpr (std::is_same_v<bool, T>) {
        cur += sizeof(T);
        return m_buf[i] != 0;
    }
    else if constexpr (std::is_same_v<double, T>) {
        return std::bit_cast<double>(Deserialize<uint64_t>(cur));
    }
    else if constexpr (std::is_enum_v<T>) {
        return static_cast<T>(Deserialize<std::underlying_type_t<T>>(cur));
    }
}

void Team::_Store(sol::state_view lua, const sol::object& o)
{
    sol::type tp = o.get_type();
    if (tp == sol::type::number) {
        Serialize(tp);
        Serialize(o.as<double>());
    }
    else if (tp == sol::type::boolean) {
        Serialize(tp);
        Serialize(o.as<bool>());
    }
    else if (tp == sol::type::string) {
        Serialize(tp);
        Serialize(o.as<std::string>());
    }
    else if (tp == sol::type::table) {
        Serialize(tp);
        Serialize(static_cast<std::size_t>(o.as<sol::table>().size()));
        for (const auto& [k, v] : o.as<sol::table>()) {
            _Store(lua, k);
            _Store(lua, v);
        }
    }
}

void Team::Store(sol::state_view lua, const sol::object& o)
{
    std::size_t attempt = 3;
    while (--attempt != 0) {
        try {
            _Store(lua, o);
            break;
        }
        catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    if (attempt == 0) { throw std::runtime_error("Failed to store value"); }
}

std::optional<sol::object> Team::_Load(sol::state_view lua, std::size_t& cur)
{
    auto tp = Deserialize<sol::type>(cur);
    if (tp == sol::type::number) { return make_object(lua, Deserialize<double>(cur)); }
    else if (tp == sol::type::boolean) {
        return make_object(lua, Deserialize<bool>(cur));
    }
    else if (tp == sol::type::string) {
        return make_object(lua, Deserialize<std::string>(cur));
    }
    else if (tp == sol::type::table) {
        auto t    = lua.create_table();
        auto size = Deserialize<std::size_t>(cur);
        for (std::size_t i = 0; i < size; ++i) {
            auto k = _Load(lua, cur);
            if (!k) { throw std::runtime_error("missing key"); }
            auto v = _Load(lua, cur);
            if (!v) { throw std::runtime_error("missing value"); }
            t[k] = v;
        }
        return t;
    }
    return {};
}

std::optional<sol::object> Team::Load(sol::state_view lua)
{
    std::size_t                attempt = 3;
    std::optional<sol::object> value;
    while (--attempt != 0) {
        try {
            std::size_t cur = 0;
            value           = _Load(lua, cur);
            break;
        }
        catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    if (attempt == 0) { throw std::runtime_error("Failed to store value"); }

    return value;
}
}    // namespace Frasy::Lua
