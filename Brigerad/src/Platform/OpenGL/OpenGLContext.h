#pragma once

#include "Brigerad/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Brigerad
{
class OpenGLContext : public GraphicsContext
{
public:
    OpenGLContext(GLFWwindow* windowHandle);

    virtual void Init() override;
    virtual void SwapBuffers() override;
private:
    GLFWwindow* m_windowHandle;
};
}