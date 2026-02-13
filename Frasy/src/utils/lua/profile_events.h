/**
 * @file    profile_events.h
 * @author  Samuel Martel
 * @date    2024-08-07
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


#ifndef PROFILE_EVENTS_H
#define PROFILE_EVENTS_H

#include <Brigerad/Core/Core.h>
#include <Brigerad/Core/Log.h>

#include <utils/string_utils.h>

#include <chrono>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <numeric>
#include <string>

#include <Windows.h>

#include "processthreadsapi.h"

namespace Frasy {
/**
 * Identification Information of a profile event.
 */
struct ProfileEventHeader {
    std::string name;
    std::string source;
    int         currentLine = 0;

    auto operator<=>(const ProfileEventHeader&) const = default;
};

struct ProfileEventMarkers {
    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    std::chrono::time_point<std::chrono::steady_clock> end   = std::chrono::steady_clock::now();
    std::chrono::microseconds                          delta = std::chrono::microseconds(0);
};

struct ProfileEvent {
    ProfileEventHeader              header;
    int                             hitCount  = 0;
    std::chrono::microseconds       totalTime = std::chrono::microseconds(0);
    std::chrono::microseconds       minTime   = std::chrono::microseconds((std::numeric_limits<int64_t>::max)());
    std::chrono::microseconds       maxTime   = std::chrono::microseconds((std::numeric_limits<int64_t>::min)());
    std::chrono::microseconds       avgTime   = std::chrono::microseconds(0);
    std::deque<ProfileEventMarkers> history;    //! Need fast insert/erase on front and back + pointer stability.

    ProfileEvent*           parent = nullptr;
    std::list<ProfileEvent> childs;    //! List because of constant insertion at end + pointer stability.
};

class ProfilerDetails {
public:
    const std::string&             label() const { return m_label; }
    const std::thread::id&         threadId() const { return m_threadId; }
    const std::list<ProfileEvent>& getEvents() const { return m_events; }
    std::chrono::microseconds      getTotalTime() const
    {
        return std::accumulate(
          m_events.begin(),
          m_events.end(),
          std::chrono::microseconds(0),
          [](std::chrono::microseconds tot, const ProfileEvent& event) { return tot + event.totalTime; });
    }

    void reset() { m_events.clear(); }

private:
    friend class Profiler;
    ProfileEvent*           m_activeEvent = nullptr;
    std::list<ProfileEvent> m_events;    //! List because of constant insertion at end + pointer stability.
    std::thread::id         m_threadId = std::this_thread::get_id();

#if defined(__cpp_lib_formatters) && __cpp_lib_formatters >= 202302L
    std::string m_label = [] {
        // Get the thread's description, if one was set. Otherwise, make a label that uses the thread's ID.
        HANDLE  currentThread = GetCurrentThread();
        PWSTR   data          = nullptr;
        HRESULT hr            = GetThreadDescription(currentThread, &data);
        if (SUCCEEDED(hr) && std::wcslen(data) > 0) {
            std::string label = StringUtils::WStringToString(data);
            LocalFree(data);
            return label;
        }
        return std::format("Thread {}", std::this_thread::get_id());
    }();
#else
    std::string m_label = std::format("Thread {}", std::this_thread::get_id()._Get_underlying_id());
#endif
};

class Profiler {
public:
    static Profiler& get()
    {
        static Profiler s_instance;
        return s_instance;
    }

    void reset() { m_events.clear(); }
    void reset(std::thread::id id) { m_events[id].reset(); }
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }

    const std::map<std::thread::id, ProfilerDetails>& getEvents() const { return m_events; }

    void reportCallEvent(const ProfileEventHeader& header)
    {
        if (!m_enabled) { return; }
        auto isEvent = [&header](const ProfileEvent& event) { return event.header == header; };

        auto  id     = std::this_thread::get_id();
        auto& events = m_events[id];

        if (events.m_activeEvent == nullptr) {
            // We're at the top level! Check if we've already seen that event before.
            if (auto it = std::ranges::find_if(events.m_events, isEvent); it != events.m_events.end()) {
                events.m_activeEvent = &*it;
            }
            else {
                // Event doesn't exist.
                events.m_events.emplace_back(header);
                events.m_activeEvent = &events.m_events.back();
            }
            events.m_activeEvent->history.emplace_back(std::chrono::steady_clock::now());
        }
        else {
            // We're a child of someone, create a new child if we've never seen it before.
            auto* parent = events.m_activeEvent;
            if (auto it = std::ranges::find_if(events.m_activeEvent->childs, isEvent);
                it != events.m_activeEvent->childs.end()) {
                events.m_activeEvent = &*it;
            }
            else {
                events.m_activeEvent->childs.emplace_back(header);
                events.m_activeEvent         = &events.m_activeEvent->childs.back();
                events.m_activeEvent->parent = parent;
            }
            events.m_activeEvent->history.emplace_back(std::chrono::steady_clock::now());
        }
    }

    void reportReturnEvent([[maybe_unused]] const ProfileEventHeader& header)
    {
        if (!m_enabled) { return; }
        auto id = std::this_thread::get_id();

        auto& event = m_events[id];
        if (event.m_activeEvent == nullptr) {
            BR_LUA_WARN("No active event to return from!");
            return;
        }

        ++event.m_activeEvent->hitCount;
        auto& marker = event.m_activeEvent->history.back();
        marker.end   = std::chrono::steady_clock::now();
        marker.delta = std::chrono::duration_cast<std::chrono::microseconds>(marker.end - marker.start);
        event.m_activeEvent->totalTime += marker.delta;
        event.m_activeEvent->minTime = (std::min)(event.m_activeEvent->minTime, marker.delta);
        event.m_activeEvent->maxTime = (std::max)(event.m_activeEvent->maxTime, marker.delta);
        event.m_activeEvent->avgTime = event.m_activeEvent->totalTime / event.m_activeEvent->hitCount;

        // Prune old data from the history.
        while (event.m_activeEvent->history.size() > s_maxHistorySize) {
            event.m_activeEvent->history.pop_front();
        }

        // TODO Sorting in here ain't exactly the least intrusive approach. It sure is the easiest tho
        // Sort the whole branch back to the root
        for (auto* parent = event.m_activeEvent->parent; parent != nullptr; parent = parent->parent) {
            parent->childs.sort(m_sortFunction);
        }
        event.m_events.sort(m_sortFunction);

        event.m_activeEvent = event.m_activeEvent->parent;
    }

    static bool sortByHitCountAsc(const ProfileEvent& a, const ProfileEvent& b) { return a.hitCount < b.hitCount; }
    static bool sortByHitCountDesc(const ProfileEvent& a, const ProfileEvent& b) { return a.hitCount > b.hitCount; }
    static bool sortByTotalTimeAsc(const ProfileEvent& a, const ProfileEvent& b) { return a.totalTime < b.totalTime; }
    static bool sortByTotalTimeDesc(const ProfileEvent& a, const ProfileEvent& b) { return a.totalTime > b.totalTime; }

    void setSortFunction(
      const std::function<bool(const ProfileEvent&, const ProfileEvent&)>& func = sortByTotalTimeDesc)
    {
        BR_ASSERT(func, "Function is not callable");
        m_sortFunction = func;
    }

private:
    bool                                       m_enabled = true;
    std::map<std::thread::id, ProfilerDetails> m_events;

    std::function<bool(const ProfileEvent&, const ProfileEvent&)> m_sortFunction = sortByTotalTimeDesc;

    static constexpr size_t s_maxHistorySize = 10'000;
};

class ProfileEventMarker {
public:
    ProfileEventMarker(std::string name, std::string source, int line)
    : m_header {std::move(name), std::move(source), line}
    {
        Profiler::get().reportCallEvent(m_header);
    }
    ~ProfileEventMarker() { Profiler::get().reportReturnEvent(m_header); }

private:
    ProfileEventHeader m_header;
};
}    // namespace Frasy

#define FRASY_PROFILE 1

#if defined(FRASY_PROFILE)
/**
 * @brief Measure the execution time of the current scope.
 */
#    define FRASY_PROFILE_SCOPE_NAME_HELPER(line) timer##line
#    define FRASY_PROFILE_SCOPE_NAME(line)        FRASY_PROFILE_SCOPE_NAME_HELPER(line)
#    define FRASY_PROFILE_SCOPE(name)                                                                                  \
        ::Frasy::ProfileEventMarker FRASY_PROFILE_SCOPE_NAME(__LINE__)(name, __FILE__, __LINE__)
/**
 * @brief Measure the execution time of the current function.
 */
#    define FRASY_PROFILE_FUNCTION() FRASY_PROFILE_SCOPE(FUNCSIG)
#else
#    define FRASY_PROFILE_SCOPE(name)
#    define FRASY_PROFILE_FUNCTION()
#endif

#endif    // PROFILE_EVENTS_H
