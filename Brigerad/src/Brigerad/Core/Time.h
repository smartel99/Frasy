#pragma once

#include "Platform/Linux/LinuxTime.h"
#include "Platform/Windows/WindowsTime.h"

namespace Brigerad
{
static double GetTime()
{
#if BR_PLATFORM_WINDOWS
    return WindowsGetTime();
#elif defined(BR_PLATFORM_LINUX)
    return LinuxGetTime();
#else
#    error Unsuported OS
#endif
}

}    // namespace Brigerad
