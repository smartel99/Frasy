#pragma once
#if defined(BR_PLATFORM_WINDOWS)
#    include "Brigerad/Core/Window.h"
#    include "Brigerad/Renderer/GraphicsContext.h"

#    include <GLFW/glfw3.h>

#    include <windows.h>

#    include <thread>

// struct GLFWindow;

namespace Brigerad {
class WindowsWindow : public Window {
public:
    WindowsWindow(const WindowProps& props);
    ~WindowsWindow() override;

    void OnUpdate() override;

    unsigned int GetWidth() const override { return m_data.width; }
    unsigned int GetHeight() const override { return m_data.height; }

    // Window attributes.
    void SetEventCallback(const EventCallbackFn& callback) override { m_data.eventCallback = callback; }

    void SetVSync(bool enabled) override;
    bool IsVSync() const override;

    void* GetNativeWindow() const override { return reinterpret_cast<void*>(m_window); }
    void  Maximize() override;
    void  Restore() override;

private:
    virtual void Init(const WindowProps& props);
    virtual void Shutdown();

private:
    GLFWwindow* m_window;

    Scope<GraphicsContext> m_context;

    struct WindowData {
        std::string  title;
        unsigned int width = 0, height = 0;
        bool         vsync = false;

        EventCallbackFn eventCallback;
    };

    HWND        m_messageWindow;
    std::jthread m_messageThread;
    WindowData  m_data;

    bool m_hasChangedTimerPeriod = false;
    unsigned int m_timerPeriod = 0;
};
}    // namespace Brigerad
#endif
