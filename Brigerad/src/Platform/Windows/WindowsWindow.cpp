#include "WindowsWindow.h"

#include "../../Brigerad/Core/Input.h"
#include "../../Brigerad/Debug/Instrumentor.h"
#include "../../Brigerad/Events/ApplicationEvent.h"
#include "../../Brigerad/Events/KeyEvents.h"
#include "../../Brigerad/Events/MouseEvent.h"
#include "../../Brigerad/Renderer/Renderer.h"
#include "../OpenGL/OpenGLContext.h"


// Device handling code from: https://learn.microsoft.com/en-us/windows/win32/devio/registering-for-device-notification
#include <dbt.h>
#include <windows.h>

namespace Brigerad {

static uint8_t s_GLFWWindowCount = 0;

namespace {
void GLFWErrorCallback(int error, const char* description)
{
    BR_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
}

bool doRegisterDeviceInterfaceToHwnd(IN GUID interfaceClassGuid, IN HWND hWnd, OUT HDEVNOTIFY* hDeviceNotify)
// Routine Description:
//     Registers an HWND for notification of changes in the device interfaces
//     for the specified interface class GUID.

// Parameters:
//     InterfaceClassGuid - The interface class GUID for the device
//         interfaces.

//     hWnd - Window handle to receive notifications.

//     hDeviceNotify - Receives the device notification handle. On failure,
//         this value is NULL.

// Return Value:
//     If the function succeeds, the return value is TRUE.
//     If the function fails, the return value is FALSE.

// Note:
//     RegisterDeviceNotification also allows a service handle be used,
//     so a similar wrapper function to this one supporting that scenario
//     could be made from this template.
{
    DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

    ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
    NotificationFilter.dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    NotificationFilter.dbcc_classguid  = interfaceClassGuid;

    *hDeviceNotify = RegisterDeviceNotification(hWnd,                          // events recipient
                                                &NotificationFilter,           // type of device
                                                DEVICE_NOTIFY_WINDOW_HANDLE    // type of recipient handle
    );

    if (*hDeviceNotify == nullptr) {
        BR_CORE_ERROR("RegisterDeviceNotification failed");
        return false;
    }

    return true;
}

INT_PTR WINAPI messageHandlerCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT           lRet = 1;
    static HDEVNOTIFY hDeviceNotify;
    static HWND       hEditWnd;
    static ULONGLONG  msgCount = 0;

    static GUID WceusbshGUID = {
      0x4d36e978,
      0xe325,
      0x11c3,
      0xbf,
      0xc1,
      0x08,
      0x00,
      0x2b,
      0xe1,
      0x03,
      0x18,
    };

    switch (message) {
        case WM_CREATE:
            //
            // This is the actual registration., In this example, registration
            // should happen only once, at application startup when the window
            // is created.
            //
            // If you were using a service, you would put this in your main code
            // path as part of your service initialization.
            //
            if (!doRegisterDeviceInterfaceToHwnd(WceusbshGUID, hWnd, &hDeviceNotify)) {
                BR_CORE_ERROR("doRegisterDeviceInterfaceToHwnd failed");
            }

            break;

        case WM_DEVICECHANGE: {
            //
            // This is the actual message from the interface via Windows messaging.
            // This code includes some additional decoding for this particular device type
            // and some common validation checks.
            //
            // Note that not all devices utilize these optional parameters in the same
            // way. Refer to the extended information for your particular device type
            // specified by your GUID.
            //
            PDEV_BROADCAST_DEVICEINTERFACE b = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
            // Output some messages to the window.
            switch (wParam) {
                    // TODO dispatch Brigerad events.
                case DBT_DEVICEARRIVAL:
                    msgCount++;
                    BR_CORE_DEBUG("Message {}: DBT_DEVICEARRIVAL", msgCount);
                    break;
                case DBT_DEVICEREMOVECOMPLETE:
                    msgCount++;
                    BR_CORE_DEBUG("Message {}, DBT_DEVICEREMOVECOMPLETE", msgCount);
                    break;
                case DBT_DEVNODES_CHANGED:
                    msgCount++;
                    BR_CORE_DEBUG("Message {}, DBT_DEVNODES_CHANGED", msgCount);
                    break;
                default:
                    msgCount++;
                    BR_CORE_DEBUG(
                      "Message {}: WM_DEVICECHANGE message received, value {} unhandled.", msgCount, wParam);
                    break;
            }
        } break;
        case WM_CLOSE:
            if (!UnregisterDeviceNotification(hDeviceNotify)) { BR_CORE_ERROR("UnregisterDeviceNotification failed"); }
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY: PostQuitMessage(0); break;

        default:
            // Send all other messages on to the default windows handler.
            lRet = DefWindowProc(hWnd, message, wParam, lParam);
            break;
    }

    return lRet;
}

bool initWindowClass()
{
    WNDCLASSEX wndClass {};
    wndClass.cbSize        = sizeof(WNDCLASSEX);
    wndClass.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wndClass.hInstance     = reinterpret_cast<HINSTANCE>(GetModuleHandle(nullptr));
    wndClass.lpfnWndProc   = reinterpret_cast<WNDPROC>(messageHandlerCallback);
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = 0;
    wndClass.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    wndClass.hbrBackground = CreateSolidBrush(RGB(192, 192, 192));
    wndClass.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wndClass.lpszClassName = TEXT("MessageHandler");
    wndClass.lpszMenuName  = nullptr;
    wndClass.hIconSm       = wndClass.hIcon;


    if (!RegisterClassEx(&wndClass)) {
        BR_CORE_ERROR("Unable to register MessageHandler class in win32!");
        return false;
    }
    return true;
}
}    // namespace

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

    if (s_GLFWWindowCount == 0) {
        BR_PROFILE_SCOPE("glfwInit");
        int success = glfwInit();
        BR_CORE_ASSERT(success, "Could not initialize GLFW!");
        glfwSetErrorCallback(GLFWErrorCallback);
    }

    {
        BR_PROFILE_SCOPE("glfwCreateWindow");
#if defined(BR_DEBUG)
        if (Renderer::GetAPI() == RendererAPI::API::OpenGL) { glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE); }
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
    glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
        data.width       = width;
        data.height      = height;

        WindowResizeEvent event(width, height);
        data.eventCallback(event);
    });

    glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
        WindowData&      data = *(WindowData*)glfwGetWindowUserPointer(window);
        WindowCloseEvent event;
        data.eventCallback(event);
    });

    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        switch (action) {
            case GLFW_PRESS: {
                KeyPressedEvent event(static_cast<KeyCode>(key), 0);
                data.eventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                KeyReleasedEvent event(static_cast<KeyCode>(key));
                data.eventCallback(event);
                break;
            }
            case GLFW_REPEAT: {
                KeyPressedEvent event(static_cast<KeyCode>(key), 1);
                data.eventCallback(event);
                break;
            }
        }
    });

    glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int keycode) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        KeyTypedEvent event(static_cast<KeyCode>(keycode));
        data.eventCallback(event);
    });

    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        switch (action) {
            case GLFW_PRESS: {
                MouseButtonPressedEvent event(static_cast<MouseCode>(button));
                data.eventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                MouseButtonReleasedEvent event(static_cast<MouseCode>(button));
                data.eventCallback(event);
                break;
            }
        }
    });

    glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xOffset, double yOffset) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        MouseScrolledEvent event((float)xOffset, (float)yOffset);
        data.eventCallback(event);
    });

    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xPos, double yPos) {
        WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

        MouseMovedEvent event((float)xPos, (float)yPos);
        data.eventCallback(event);
    });


    m_messageThread = std::jthread([this](std::stop_token stoken) {
        if (!initWindowClass()) { return; }

        m_messageWindow = CreateWindowEx(WS_EX_APPWINDOW,
                                         TEXT("MessageHandler"),
                                         TEXT("MessageHandler"),
                                         0,
                                         0,
                                         0,
                                         0,
                                         0,
                                         HWND_MESSAGE,
                                         nullptr,
                                         nullptr,
                                         nullptr);

        if (m_messageWindow == nullptr) {
            BR_CORE_ERROR("Unable to create message handler window!");
            return;
        }

        MSG msg;
        int retVal = 0;
        while (!stoken.stop_requested() && (retVal = GetMessage(&msg, m_messageWindow, 0, 0)) != 0) {
            if (retVal == -1) {
                BR_CORE_ERROR("GetMessage returned -1");
                return;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    });
}

void WindowsWindow::Shutdown()
{
    BR_PROFILE_FUNCTION();

    glfwDestroyWindow(m_window);
    --s_GLFWWindowCount;

    if (s_GLFWWindowCount == 0) {
        if (m_messageWindow != nullptr) {
            m_messageThread.request_stop();
            SendMessage(m_messageWindow, WM_CLOSE, {}, {});
        }
        glfwTerminate();
    }
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
    else {
        glfwSwapInterval(0);
    }

    m_data.vsync = enabled;
}

bool WindowsWindow::IsVSync() const
{
    return m_data.vsync;
}

}    // namespace Brigerad
