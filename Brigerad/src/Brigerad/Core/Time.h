#pragma once

#include "Platform/Windows/WindowsTime.h"
#include "Platform/Linux/LinuxTime.h"

namespace Brigerad
{
double GetTime()
{
#if BR_PLATFORM_WINDOWS
    return WindowsGetTime();
#elif defined(BR_PLATFORM_LINUX)
    return LinuxGetTime();
#else
#error Unsuported OS
#endif
}

} // namespace Brigerad