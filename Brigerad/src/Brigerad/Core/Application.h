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

    void run();

    void onEvent(Event& e);

    void pushLayer(Layer* layer);
    void pushOverlay(Layer* layer);

    void popLayer(Layer* layer);

    void close();

    Window& getWindow() { return *m_window; }

    ImGuiLayer* getImGuiLayer() { return m_imguiLayer; }

    void queuePostFrameTask(const std::function<void()>& fn)
    {
        if (fn)
        {
            m_postFrameTasks.push_back(fn);
        }
    }

    static Application& Get() { return *s_instance; }

private:
    bool onWindowClose(WindowCloseEvent& e);
    bool onWindowResize(WindowResizeEvent& e);
    bool onKeyPressed(KeyPressedEvent& e);

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
