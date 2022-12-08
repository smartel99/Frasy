#pragma once
#if defined(BR_PLATFORM_LINUX)
#include "GLFW/glfw3.h"

namespace Brigerad
{
double LinuxGetTime()
{
    return glfwGetTime();
}
} // namespace Brigerad
#endif
