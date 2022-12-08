/**
 * @file   E:\dev\Brigerad\Brigerad\src\Platform\OpenGL\OpenGLContext.cpp
 * @author Samuel Martel
 * @date   2020/03/06
 *
 * @brief  Source for the OpenGLContext module.
 */
#include "OpenGLContext.h"

#include "Brigerad/Core/Log.h"
#include "Brigerad/Debug/Instrumentor.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <string_view>

namespace Brigerad
{

OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
    : m_windowHandle(windowHandle)
{
    BR_CORE_ASSERT(windowHandle, "Window handle is null!");
}

void OpenGLContext::Init()
{
    BR_PROFILE_FUNCTION();

    glfwMakeContextCurrent(m_windowHandle);

    // Glad init stuff.
    int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    BR_CORE_ASSERT(status, "Failed to initialize Glad!");

    BR_CORE_INFO("OpenGL Info:");
    BR_CORE_INFO("  Vendor: {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    BR_CORE_INFO("  Renderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    BR_CORE_INFO("  Version: {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    #if BR_ENABLE_ASSERTS
    int versionMajor;
    int versionMinor;

    glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
    glGetIntegerv(GL_MINOR_VERSION, &versionMinor);

    BR_CORE_ASSERT(versionMajor > 4 || (versionMajor == 4 && versionMinor >= 5),
                   "Brigerad requires at least OpenGL version 4.5!");
    #endif
}

void OpenGLContext::SwapBuffers()
{
    BR_PROFILE_FUNCTION();
    glfwSwapBuffers(m_windowHandle);
}
}
