/**
 * @file    log_once.h
 * @brief   Sets up Brigerad logging once per test process, for code under test that logs.
 */

#ifndef FRASY_TESTS_REPORT_LOG_ONCE_H
#define FRASY_TESTS_REPORT_LOG_ONCE_H

#include <Brigerad/Core/Log.h>

/** Initialising twice in one process makes the rotating file sink fail, so every fixture goes through here. */
inline void initLogOnce()
{
    static const bool s_initialised = [] {
        Brigerad::Log::Init(false, true);
        return true;
    }();
    (void)s_initialised;
}

#endif    // FRASY_TESTS_REPORT_LOG_ONCE_H
