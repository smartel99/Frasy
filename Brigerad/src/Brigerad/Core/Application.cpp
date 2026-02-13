/**
 * @file Application.cpp
 * @author Samuel Martel (martelsamuel00@gmail.com)
 * @brief   Header for the Application module.
 * @version 0.1
 * @date    2020-05-14
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "Application.h"

#include "../Renderer/asset_manager.h"
#include "../Script/ScriptEngine.h"
#include "Input.h"
#include "KeyCodes.h"
#include "Time.h"

namespace Brigerad {
#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

/**
 * @brief   Singleton instance of the running application
 *
 */
Application* Application::s_instance = nullptr;

/**
 * @brief   Construct a new Application:: Application object
 *          This creates a new window and binds the event function to it.
 */
Application::Application(const std::string& name, bool maximized)
{
    BR_PROFILE_FUNCTION();

    // Don't allow multiple instances of Application.
    BR_CORE_ASSERT(!s_instance, "Application already exists!");
    s_instance = this;

    // Create the window for the application.
    m_window = Scope<Window>(Window::Create(WindowProps(name, maximized)));
    // Bind the Application's events to the window's.
    m_window->SetEventCallback([this](Event& e) { onEvent(e); });

    // Initialize the rendering pipeline.
    Renderer::Init();

    // Initialize the Lua scripting engine.
    ScriptEngine::Init();

    // Initialize all ImGui related things.
    m_imguiLayer = new ImGuiLayer();

    // Add the ImGui layer to the layer stack as an overlay (on top of everything).
    m_layerStack.PushOverlay(m_imguiLayer);
    m_imguiLayer->onAttach();
    AssetManager::Init();
}

/**
 * @brief   Destroy the Application:: Application object
 */
Application::~Application()
{
    // Gracefully shut down the scripting engine.
    ScriptEngine::Shutdown();
}

/**
 * @brief   The main run loop of the application, where all layers are updated.
 *
 * @param   None
 * @retval  None
 */
void Application::run()
{
    BR_PROFILE_FUNCTION();

    // For as long as the application should be running:
    while (m_running) {
        BR_PROFILE_SCOPE("RunLoop");

        // Get the time elapsed since the last frame.
        float    time     = static_cast<float>(GetTime());
        Timestep timestep = time - m_lastFrameTime;
        m_lastFrameTime   = time;

        for (auto& gif : AssetManager::m_gifs | std::views::values) {
            gif->onUpdate(timestep);
        }

        // If the window is not minimized:
        // (If the window is minimized, we don't want to waste time rendering stuff!)
        if (!m_minimized) {
            {
                // Update all Application Layers.
                BR_PROFILE_SCOPE("Layer Stack onUpdate");
                for (Layer* layer : m_layerStack) {
                    layer->onUpdate(timestep);
                }
            }

            // Render all ImGui Layers.
            m_imguiLayer->Begin();
            {
                BR_PROFILE_SCOPE("LayerStack onImGuiRender");
                for (Layer* layer : m_layerStack) {
                    layer->onImGuiRender();
                }
            }
            m_imguiLayer->End();
        }

        // Do the per-frame window updating tasks.
        m_window->OnUpdate();

        // Execute the post-frame task queue.
        for (const auto& task : m_postFrameTasks) {
            task();
        }
        m_postFrameTasks.clear();
    }

    for (auto&& layer : m_layerStack) {
        layer->onDetach();
    }
}

/**
 * @brief   Callback function for all events happening in the application.
 *          It dispatches and propagates the event through all layers until it is handled.
 *
 * @param   e The event to be dispatched.
 */
void Application::onEvent(Event& e)
{
    BR_PROFILE_FUNCTION();

    // Creates a dispatch context with the event.
    EventDispatcher dispatcher(e);
    // Dispatch it to the proper handling function in the Application, if the type matches.
    dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return onWindowClose(e); });
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return onWindowResize(e); });
    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) { return onKeyPressed(e); });


    // For each layers in the layer stack, from the last one to the first:
    for (auto it = m_layerStack.end(); it != m_layerStack.begin();) {
        // Pass the event to the layer.
        (*--it)->onEvent(e);
        // If the event has been handled by that layer:
        if (e.Handled()) {
            // Stop propagating it.
            break;
        }
    }
}


/**
 * @brief   Push a new layer at the back of the layer stack.
 *
 * @param   layer A pointer to the layer
 * @retval  None
 */
void Application::pushLayer(Layer* layer)
{
    BR_PROFILE_FUNCTION();

    std::function<void()> task = [this, layer]() {
        // Push the new layer to the stack.
        m_layerStack.PushLayer(layer);
        // Initialize the layer.
        layer->onAttach();
    };

    m_postFrameTasks.push_back(task);
}

/**
 * @brief   Push a new layer at the front of the layer stack.
 *
 * @param   layer A pointer to the layer
 * @retval  None
 */
void Application::pushOverlay(Layer* layer)
{
    BR_PROFILE_FUNCTION();

    std::function<void()> task = [this, layer]() {
        // Push the new layer to the stack.
        m_layerStack.PushOverlay(layer);
        // Initialize the layer.
        layer->onAttach();
    };

    m_postFrameTasks.push_back(task);
}

void Application::popLayer(Layer* layer)
{
    BR_PROFILE_FUNCTION();
    BR_CORE_ASSERT(layer != nullptr, "Layer is NULL in Application::popLayer");

    std::function<void()> task = [this, layer]() {
        // Pop the layer from the stack.
        m_layerStack.PopLayer(layer);
        // De-initialize the layer.
        layer->onDetach();
    };

    m_postFrameTasks.push_back(task);
}

void Application::close()
{
    m_running = false;
}

/**
 * @brief   Handle the window close event.
 *          This event happens whenever the main application window closes.
 *
 * @param   e The event
 * @retval  Always return true
 */
bool Application::onWindowClose([[maybe_unused]] WindowCloseEvent& e)
{
    m_running = false;
    return true;
}

/**
 * @brief   Handle the window resize event.
 *
 * @param   e
 * @retval  true
 * @retval  false
 */
bool Application::onWindowResize(WindowResizeEvent& e)
{
    BR_PROFILE_FUNCTION();

    if (e.GetHeight() == 0 || e.GetWidth() == 0) {
        m_minimized = true;
        return false;
    }
    m_minimized = false;

    Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

    return false;
}

bool Application::onKeyPressed(KeyPressedEvent& e)
{
    BR_PROFILE_FUNCTION();

    if (e.GetKeyCode() == BR_KEY_ESCAPE && e.GetRepeatCount() == 0) {
        m_imguiLayer->ToggleIsVisible();
        return true;
    }
    return false;
}

}    // namespace Brigerad
