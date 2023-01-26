#pragma once
#if defined(BR_PLATFORM_WINDOWS)
#    include "GLFW/glfw3.h"

namespace Brigerad
{
static double WindowsGetTime()
{
    return glfwGetTime();
}
}    // namespace Brigerad
#endif
