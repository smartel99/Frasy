#pragma once
#include "Brigerad/Core/Core.h"
#include "Brigerad/Events/Event.h"
#include "Brigerad/Core/LayerStack.h"
#include "Brigerad/Events/ApplicationEvent.h"

#include "Brigerad/Core/Window.h"

#include "Brigerad/ImGui/ImGuiLayer.h"

#include "Brigerad/Core/Timestep.h"

#include "Brigerad/Renderer/Shader.h"
#include "Brigerad/Renderer/Buffer.h"
#include "Brigerad/Renderer/VertexArray.h"
#include "Brigerad/Renderer/Renderer.h"
#include "Brigerad/Renderer/OrthographicCamera.h"

namespace Brigerad
{
class BRIGERAD_API Application
{
public:
    Application(const std::string& name = "Brigerad Engine");
    virtual ~Application();

    void Run();

    void OnEvent(Event& e);

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    void PopLayer(Layer* layer);

    void Close();

    inline Window& GetWindow() { return *m_window; }

    inline ImGuiLayer* GetImGuiLayer() { return m_imguiLayer; }

    inline void QueuePostFrameTask(const std::function<void()>& fn)
    {
        if (fn)
        {
            m_postFrameTasks.push_back(fn);
        }
    }

    inline static Application& Get() { return *s_instance; }

private:
    bool OnWindowClose(WindowCloseEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);
    bool OnKeyPressed(KeyPressedEvent& e);

private:
    Scope<Window> m_window;
    ImGuiLayer*   m_imguiLayer;

    bool       m_running   = true;
    bool       m_minimized = false;
    LayerStack m_layerStack;

    float m_lastFrameTime = 0.0f;

    std::vector<std::function<void()>> m_postFrameTasks;

private:
    static Application* s_instance;
};

// To be defined in client.
Application* CreateApplication();
}    // namespace Brigerad
