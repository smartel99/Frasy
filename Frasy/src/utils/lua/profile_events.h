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

#include <chrono>
#include <functional>
#include <list>
#include <numeric>
#include <string>

namespace Frasy {
/**
 * Identification Information of a profile event.
 */
struct ProfileEventHeader {
    std::string name;
    std::string source;
    int         currentLine = 0;

    constexpr auto operator<=>(const ProfileEventHeader&) const = default;
};

struct ProfileEvent {
    ProfileEventHeader                                 header;
    int                                                hitCount      = 0;
    std::chrono::time_point<std::chrono::steady_clock> lastStartTime = std::chrono::steady_clock::now();
    std::chrono::microseconds                          totalTime     = std::chrono::microseconds(0);
    std::chrono::microseconds minTime = std::chrono::microseconds(std::numeric_limits<int64_t>::max());
    std::chrono::microseconds maxTime = std::chrono::microseconds(std::numeric_limits<int64_t>::min());
    std::chrono::microseconds avgTime = std::chrono::microseconds(0);

    ProfileEvent*           parent = nullptr;
    std::list<ProfileEvent> childs;
};

class Profiler {
public:
    static Profiler& get()
    {
        static Profiler s_instance;
        return s_instance;
    }

    void                             reset() { m_events.clear(); }
    const std::list<ProfileEvent>&   getEvents() const { return m_events; }
    std::chrono::microseconds getTotalTime() const
    {
        return std::accumulate(
          m_events.begin(),
          m_events.end(),
          std::chrono::microseconds(0),
          [](std::chrono::microseconds tot, const ProfileEvent& event) { return tot + event.totalTime; });
    }

    void reportCallEvent(const ProfileEventHeader& header)
    {
        auto isEvent = [&header](const ProfileEvent& event) { return event.header == header; };
        if (m_activeEvent == nullptr) {
            // We're at the top level! Check if we've already seen that event before.
            if (auto it = std::ranges::find_if(m_events, isEvent); it != m_events.end()) {
                m_activeEvent                = &*it;
                m_activeEvent->lastStartTime = std::chrono::steady_clock::now();
            }
            else {
                // Event doesn't exist.
                m_events.emplace_back(header);
                m_activeEvent = &m_events.back();
            }
        }
        else {
            // We're a child of someone, create a new child if we've never seen it before.
            auto* parent = m_activeEvent;
            if (auto it = std::ranges::find_if(m_activeEvent->childs, isEvent); it != m_activeEvent->childs.end()) {
                m_activeEvent                = &*it;
                m_activeEvent->lastStartTime = std::chrono::steady_clock::now();
            }
            else {
                m_activeEvent->childs.emplace_back(header);
                m_activeEvent         = &m_activeEvent->childs.back();
                m_activeEvent->parent = parent;
            }
        }
    }

    void reportReturnEvent(const ProfileEventHeader& header)
    {
        if (m_activeEvent == nullptr) {
            BR_LUA_WARN("Returning from a function that was never called!");
            return;
        }

        ++m_activeEvent->hitCount;
        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() -
                                                                           m_activeEvent->lastStartTime);
        m_activeEvent->totalTime += delta;
        m_activeEvent->minTime = std::min(m_activeEvent->minTime, delta);
        m_activeEvent->maxTime = std::max(m_activeEvent->maxTime, delta);
        m_activeEvent->avgTime = m_activeEvent->totalTime / m_activeEvent->hitCount;

        // Sort the whole branch back to the root
        for (auto* parent = m_activeEvent->parent; parent != nullptr; parent = parent->parent) {
            parent->childs.sort(m_sortFunction);
        }
        m_events.sort(m_sortFunction);

        m_activeEvent = m_activeEvent->parent;
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
    ProfileEvent*           m_activeEvent = nullptr;
    std::list<ProfileEvent> m_events;

    std::function<bool(const ProfileEvent&, const ProfileEvent&)> m_sortFunction = sortByTotalTimeDesc;
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
