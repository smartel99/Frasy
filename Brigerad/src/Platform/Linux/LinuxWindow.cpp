#include "brpch.h"
#include "LinuxWindow.h"

#include "Brigerad/Events/ApplicationEvent.h"
#include "Brigerad/Events/MouseEvent.h"
#include "Brigerad/Events/KeyEvents.h"

#include "Platform/OpenGL/OpenGLContext.h"

#if defined(BR_PLATFORM_LINUX)
namespace Brigerad
{

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* desc);

Window* Window::Create(const WindowProps& props)
{
    return new LinuxWindow(props);
}

LinuxWindow::LinuxWindow(const WindowProps& props) { Init(props); }

LinuxWindow::~LinuxWindow() { Shutdown(); }

void LinuxWindow::OnUpdate()
{
    glfwPollEvents();
    m_context->SwapBuffers();
}

void LinuxWindow::SetVSync(bool enabled)
{
    if (enabled)
    {
        glfwSwapInterval(1);
    }
    else
    {
        glfwSwapInterval(0);
    }

    m_data.vsync = enabled;
}

bool LinuxWindow::IsVSync() const { return m_data.vsync; }

void LinuxWindow::Init(const WindowProps& props)
{
    m_data.title  = props.title;
    m_data.width  = props.width;
    m_data.height = props.height;

    BR_CORE_INFO("Creating window {0} ({1}, {2})",
                 props.title,
                 props.width,
                 props.height);

    if (!s_GLFWInitialized)
    {
        // TODO: glfwTerminate on system shutdown.
        int success = glfwInit();
        BR_CORE_ASSERT(success, "Could not initialize GLFW!");
        glfwSetErrorCallback(GLFWErrorCallback);
        s_GLFWInitialized = true;
    }

    m_window =
      glfwCreateWindow((int)props.width, (int)props.height, m_data.title.c_str(), nullptr, nullptr);

    m_context = new OpenGLContext(m_window);
    m_context->Init();
    // ^

    glfwSetWindowUserPointer(m_window, &m_data);
    SetVSync(true);

    // Set GLFW callbacks.
    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        WindowData& data =
          *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));
        data.width  = width;
        data.height = height;

        WindowResizeEvent event(width, height);
        data.eventCallback(event);
    });

    glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
        WindowData& data =
          *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));
        WindowCloseEvent event;
        data.eventCallback(event);
    });

    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        WindowData& data =
          *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

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

    glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int keycode) {
        WindowData& data =
          *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));
        KeyTypedEvent event(static_cast<KeyCode>(keycode));
        data.eventCallback(event);
    });

    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mod) {
        WindowData& data =
          *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

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

    glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xOffset, double yOffset) {
        WindowData& data =
          *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

        MouseScrolledEvent event((float)xOffset, (float)yOffset);
        data.eventCallback(event);
    });

    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xPos, double yPos) {
        WindowData& data =
          *reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

        MouseMovedEvent event((float)xPos, (float)yPos);
        data.eventCallback(event);
    });
}

void LinuxWindow::Shutdown() { glfwDestroyWindow(m_window); }

void GLFWErrorCallback(int error, const char* desc)
{
    BR_CORE_ERROR("GLFW Error ({0}): {1}", error, desc);
}

}  // namespace Brigerad
#endif
