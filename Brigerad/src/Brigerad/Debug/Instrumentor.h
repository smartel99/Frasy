/**
 * @file Instrumentor.h
 * @author The Cherno
 * @brief Basic instrumentation profiler by Cherno
 * @version 0.1
 * @date 2020-05-09
 *
 * @note    Usage: Include this header file somewhere in your code (eg. precompiled header),
 *          and then use like:
 *
 *          ```
 *          Instrumentor::Get().BeginSession("Session Name");   // Begin Session.
 *          {
 *              // Place code like this in scopes you'd like to include in profiling.
 *              InstrumentationTimer timer("Profiled Scope Name");
 *              // Your code here.
 *          }
 *          Instrumentor::Get().EndSession();                   // End the Session.
 *          ```
 *
 *          You will probably want to macro-fy this, to switch on/off easily
 *          and use things like __FUNCSIG__ for the profile name.
 */
#pragma once
#include "Brigerad/Core/Core.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <string>

#include <thread>


namespace Brigerad
{
struct ProfileResult
{
    std::string Name;
    long long   Start, End;
    uint32_t    ThreadID;
};

struct InstrumentationSession
{
    std::string                                                 Name;
    std::chrono::time_point<std::chrono::high_resolution_clock> EndPoint;

    InstrumentationSession(const std::string& name, long long duration = 0) : Name(name)
    {
        EndPoint =
          duration != 0
            ? std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(duration)
            : std::chrono::time_point<std::chrono::high_resolution_clock>::max();
    }
};

class Instrumentor
{
private:
    InstrumentationSession* m_currentSession;
    std::ofstream           m_outputStream;
    int                     m_profileCount;

public:
    Instrumentor() : m_currentSession(nullptr), m_profileCount(0) {}

    void BeginSession(const std::string& name,
                      const std::string& filepath = "results.json",
                      long long          duration = 0)
    {
        m_outputStream.open(filepath);
        WriteHeader();
        m_currentSession = new InstrumentationSession {name};
    }

    void EndSession()
    {
        WriteFooter();
        m_outputStream.close();
        delete m_currentSession;
        m_currentSession = nullptr;
        m_profileCount   = 0;
    }

    void WriteProfile(const ProfileResult& result)
    {
        if (m_currentSession == nullptr) { return; }

        if (m_profileCount++ > 0) { m_outputStream << ","; }

        std::string name = result.Name;
        std::replace(name.begin(), name.end(), '"', '\'');

        m_outputStream << '{' << R"("cat":"function",)"
                       << R"("dur":)" << (result.End - result.Start) << ',' << R"("name":")" << name
                       << R"(",)"
                       << R"("ph":"X",)"
                       << R"("pid":0,)"
                       << R"("tid":)" << result.ThreadID << ',' << R"("ts":)" << result.Start
                       << '}';

        m_outputStream.flush();

        if (result.End >=
            std::chrono::time_point_cast<std::chrono::microseconds>(m_currentSession->EndPoint)
              .time_since_epoch()
              .count())
        {
            EndSession();
            return;
        }
    }

    void WriteHeader()
    {
        m_outputStream << R"({"otherData": {}, "traceEvents":[)";
        m_outputStream.flush();
    }

    void WriteFooter()
    {
        m_outputStream << "]}";
        m_outputStream.flush();
    }

    static Instrumentor& Get()
    {
        static Instrumentor instance;
        return instance;
    }
};


class InstrumentationTimer
{
public:
    InstrumentationTimer(const char* name) : m_name(name), m_stopped(false)
    {
        m_startTimepoint = GetTime();
    }

    ~InstrumentationTimer()
    {
        if (!m_stopped) { Stop(); }
    }

    void Stop()
    {
        auto endTimepoint = GetTime();

        long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimepoint)
                            .time_since_epoch()
                            .count();
        long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint)
                          .time_since_epoch()
                          .count();

        uint32_t threadID = (uint32_t)std::hash<std::thread::id> {}(std::this_thread::get_id());
        Instrumentor::Get().WriteProfile({m_name, start, end, threadID});

        m_stopped = true;
    }

private:
    const char*                                                 m_name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTimepoint;
    bool                                                        m_stopped;

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> GetTime()
    {
        static auto lastTime = std::chrono::high_resolution_clock::now();
        auto        time     = std::chrono::high_resolution_clock::now();

        if (time == lastTime) { time += std::chrono::microseconds(1); }
        lastTime = time;
        return time;
    }
};

}    // namespace Brigerad

#define BR_PROFILE 1

#if defined(BR_PROFILE)
/**
 * @brief Initialize a profiling session.
 */
#    define BR_PROFILE_BEGIN_SESSION(name, filepath)                                               \
        ::Brigerad::Instrumentor::Get().BeginSession(name, filepath)
/**
 * @brief End a profiling session and
 *        dump it into the file set in BR_PROFILE_BEGIN_SESSION.
 */
#    define BR_PROFILE_END_SESSION() ::Brigerad::Instrumentor::Get().EndSession()
/**
 * @brief Measure the execution time of the current scope.
 */
#    define BR_PROFILE_SCOPE(name) ::Brigerad::InstrumentationTimer timer##__LINE__(name)
/**
 * @brief Measure the execution time of the current function.
 */
#    define BR_PROFILE_FUNCTION() BR_PROFILE_SCOPE(FUNCSIG)
#else
#    define BR_PROFILE_BEGIN_SESSION(name, filepath)
#    define BR_PROFILE_END_SESSION()
#    define BR_PROFILE_SCOPE(name)
#    define BR_PROFILE_FUNCTION()
#endif
