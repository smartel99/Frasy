#pragma once
#if defined(BR_PLATFORM_WINDOWS)
#include "GLFW/glfw3.h"

namespace Brigerad
{
double WindowsGetTime()
{
    return glfwGetTime();
}
} // namespace Brigerad
#endif
