#include "WindowsWindow.h"

#include "../../Brigerad/Core/Input.h"
#include "../../Brigerad/Debug/Instrumentor.h"
#include "../../Brigerad/Events/ApplicationEvent.h"
#include "../../Brigerad/Events/KeyEvents.h"
#include "../../Brigerad/Events/MouseEvent.h"
#include "../../Brigerad/Renderer/Renderer.h"
#include "../OpenGL/OpenGLContext.h"

namespace Brigerad
{

static uint8_t s_GLFWWindowCount = 0;

static void GLFWErrorCallback(int error, const char* description)
{
    BR_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}

Window* Window::Create(const WindowProps& props)
{
    return new WindowsWindow(props);
}

WindowsWindow::WindowsWindow(const WindowProps& props)
{
    BR_PROFILE_FUNCTION();

    Init(props);
}

WindowsWindow::~WindowsWindow()
{
    BR_PROFILE_FUNCTION();

    Shutdown();
}

void WindowsWindow::Init(const WindowProps& props)
{
    BR_PROFILE_FUNCTION();

    m_data.title  = props.title;
    m_data.width  = props.width;
    m_data.height = props.height;

    BR_CORE_INFO("Creating window {0} ({1}, {2})", props.title, props.width, props.height);

    if (s_GLFWWindowCount == 0)
    {
        BR_PROFILE_SCOPE("glfwInit");
        int success = glfwInit();
        BR_CORE_ASSERT(success, "Could not initialize GLFW!");
        glfwSetErrorCallback(GLFWErrorCallback);
    }

    {
        BR_PROFILE_SCOPE("glfwCreateWindow");
#if defined(BR_DEBUG)
        if (Renderer::GetAPI() == RendererAPI::API::OpenGL) glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        m_window = glfwCreateWindow((int)props.width, (int)props.height, m_data.title.c_str(), nullptr, nullptr);
        ++s_GLFWWindowCount;
    }

    m_context = GraphicsContext::Create(m_window);
    m_context->Init();

    glfwSetWindowUserPointer(m_window, &m_data);
    SetVSync(true);

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(m_window,
                              [](GLFWwindow* window, int width, int height)
                              {
                                  WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
                                  data.width       = width;
                                  data.height      = height;

                                  WindowResizeEvent event(width, height);
                                  data.eventCallback(event);
                              });

    glfwSetWindowCloseCallback(m_window,
                               [](GLFWwindow* window)
                               {
                                   WindowData&      data = *(WindowData*)glfwGetWindowUserPointer(window);
                                   WindowCloseEvent event;
                                   data.eventCallback(event);
                               });

    glfwSetKeyCallback(m_window,
                       [](GLFWwindow* window, int key, int scancode, int action, int mods)
                       {
                           WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                           switch (action)
                           {
                               case GLFW_PRESS:
                               {
                                   KeyPressedEvent event(static_cast<KeyCode>(key), 0);
                                   data.eventCallback(event);
                                   break;
                               }
                               case GLFW_RELEASE:
                               {
                                   KeyReleasedEvent event(static_cast<KeyCode>(key));
                                   data.eventCallback(event);
                                   break;
                               }
                               case GLFW_REPEAT:
                               {
                                   KeyPressedEvent event(static_cast<KeyCode>(key), 1);
                                   data.eventCallback(event);
                                   break;
                               }
                           }
                       });

    glfwSetCharCallback(m_window,
                        [](GLFWwindow* window, unsigned int keycode)
                        {
                            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                            KeyTypedEvent event(static_cast<KeyCode>(keycode));
                            data.eventCallback(event);
                        });

    glfwSetMouseButtonCallback(m_window,
                               [](GLFWwindow* window, int button, int action, int mods)
                               {
                                   WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                                   switch (action)
                                   {
                                       case GLFW_PRESS:
                                       {
                                           MouseButtonPressedEvent event(static_cast<MouseCode>(button));
                                           data.eventCallback(event);
                                           break;
                                       }
                                       case GLFW_RELEASE:
                                       {
                                           MouseButtonReleasedEvent event(static_cast<MouseCode>(button));
                                           data.eventCallback(event);
                                           break;
                                       }
                                   }
                               });

    glfwSetScrollCallback(m_window,
                          [](GLFWwindow* window, double xOffset, double yOffset)
                          {
                              WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                              MouseScrolledEvent event((float)xOffset, (float)yOffset);
                              data.eventCallback(event);
                          });

    glfwSetCursorPosCallback(m_window,
                             [](GLFWwindow* window, double xPos, double yPos)
                             {
                                 WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                                 MouseMovedEvent event((float)xPos, (float)yPos);
                                 data.eventCallback(event);
                             });
}

void WindowsWindow::Shutdown()
{
    BR_PROFILE_FUNCTION();

    glfwDestroyWindow(m_window);
    --s_GLFWWindowCount;

    if (s_GLFWWindowCount == 0) { glfwTerminate(); }
}

void WindowsWindow::OnUpdate()
{
    BR_PROFILE_FUNCTION();

    glfwWaitEventsTimeout(0.1);
    m_context->SwapBuffers();
}

void WindowsWindow::SetVSync(bool enabled)
{
    BR_PROFILE_FUNCTION();

    if (enabled) { glfwSwapInterval(1); }
    else { glfwSwapInterval(0); }

    m_data.vsync = enabled;
}

bool WindowsWindow::IsVSync() const
{
    return m_data.vsync;
}

}    // namespace Brigerad
