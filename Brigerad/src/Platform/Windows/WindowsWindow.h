#pragma once
#if defined(BR_PLATFORM_WINDOWS)
#include "Brigerad/Core/Window.h"
#include "Brigerad/Renderer/GraphicsContext.h"

#include <GLFW/glfw3.h>

// struct GLFWindow;

namespace Brigerad
{
class WindowsWindow : public Window
{
public:
    WindowsWindow(const WindowProps &props);
    virtual ~WindowsWindow() override;

    void OnUpdate() override;

    inline unsigned int GetWidth() const override
    {
        return m_data.width;
    }
    inline unsigned int GetHeight() const override
    {
        return m_data.height;
    }

    // Window attributes.
    inline void SetEventCallback(const EventCallbackFn &callback) override
    {
        m_data.eventCallback = callback;
    }

    void SetVSync(bool enabled) override;
    bool IsVSync() const override;

    inline void *GetNativeWindow() const override
    {
        return reinterpret_cast<void *>(m_window);
    }

private:
    virtual void Init(const WindowProps &props);
    virtual void Shutdown();

private:
    GLFWwindow *m_window;

    Scope<GraphicsContext> m_context;

    struct WindowData
    {
        std::string title = "";
        unsigned int width = 0, height = 0;
        bool vsync = false;

        EventCallbackFn eventCallback;
    };

    WindowData m_data;
};
} // namespace Brigerad
#endif