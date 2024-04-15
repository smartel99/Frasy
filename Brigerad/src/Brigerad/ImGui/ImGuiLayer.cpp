/**
 * @file   ImGuiLayer.cpp
 * @author Samuel Martel
 * @date   2020/03/01
 *
 * @brief  Source for the ImGuiLayer module.
 */
#include "ImGuiLayer.h"

#include "../Core/Application.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <implot.h>

// TEMP
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Brigerad
{
ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer")
{
}

ImGuiLayer::~ImGuiLayer() = default;

void ImGuiLayer::onAttach()
{
    BR_PROFILE_FUNCTION();

    // Setup Dear ImGui context.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    // Enable keyboard control.
                                                             //     io.ConfigFlags |=
                                                             //     ImGuiConfigFlags_NavEnableGamepad;
                                                             //     // Enable Gamepad control.
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;        // Enable docking.
                                                             //   io.ConfigFlags |=
    //     ImGuiConfigFlags_ViewportsEnable;    // Enable multi-viewport / platform window.

    // io.Fonts->AddFontFromFileTTF("assets/fonts/OpenSans-Bold.ttf", 18.0f);
    io.FontDefault = io.Fonts->AddFontFromFileTTF(
      "assets/fonts/JetBrains Mono Regular Nerd Font Complete Windows Compatible.ttf", 18.0f);

    // Setup dear Imgui style.
    ImGui::StyleColorsDark();

    // When view ports are enabled we tweak WindowRounding/WindowBg
    // so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding            = 0.0f;
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    }

    Application& app    = Application::Get();
    GLFWwindow*  window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

    // Setup Platform/Renderer bindings.
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");
}

void ImGuiLayer::onDetach()
{
    BR_PROFILE_FUNCTION();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}

void ImGuiLayer::OnEvent(Event& event)
{
    if (m_blockImGuiEvents)
    {
        ImGuiIO& io = ImGui::GetIO();
        event.m_handled |= event.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
        event.m_handled |= event.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
    }
}

void ImGuiLayer::onImGuiRender()
{
    BR_PROFILE_FUNCTION();

    m_time = ImGui::GetTime();

    if (m_isProfiling)
    {
        if (m_time >= m_profilingStartTime + m_profilingDuration)
        {
            m_isProfiling = false;
            BR_PROFILE_END_SESSION();
        }
    }

    if (!m_open) { return; }


    if (m_showMetricWindow) { ImGui::ShowMetricsWindow(&m_showMetricWindow); }

#if defined(BR_DEBUG)
    if (m_showStyleEditor)
    {
        ImGui::Begin("Style Editor", &m_showStyleEditor);
        ImGui::ShowStyleEditor();
        ImGui::End();
    }

    if (m_showDemoWindow) { ImGui::ShowDemoWindow(&m_showDemoWindow); }
    if (m_showPlotWindow) { ImPlot::ShowDemoWindow(&m_showDemoWindow); }
#endif

    auto& window  = Application::Get().GetWindow();
    bool  isVSync = window.IsVSync();
    if (ImGui::Begin("Settings", &m_open))
    {
        float fps = ImGui::GetIO().Framerate;
        ImGui::Text("Framerate: %.02f (%.03fms)", fps, ((1.0f / fps) * 1000.0f));

        if (ImGui::Checkbox("vsync", &isVSync))
        {
            window.SetVSync(isVSync);
            BR_CORE_INFO("Set VSync to {0}", isVSync);
        }

        if (ImGui::Button("open metric window")) { m_showMetricWindow = true; }

#if defined(BR_DEBUG)
        if (ImGui::Button("open style editor")) { m_showStyleEditor = true; }
        if (ImGui::Button("Show Demo Window")) { m_showDemoWindow = true; }
        if (ImGui::Button("Show Plot Demo Window")) { m_showPlotWindow = true; }
#endif


        if (ImGui::Button(!m_isProfiling ? "Start Profiling" : "Stop  Profiling"))
        {
            if (!m_isProfiling)
            {
                m_profilingStartTime = m_profilingDuration > 0 ? m_time : std::numeric_limits<double>::max();
                BR_PROFILE_BEGIN_SESSION("Profiling Session", "BrigeradProfiling-Session.json");
                m_isProfiling = true;
            }
            else
            {
                BR_PROFILE_END_SESSION();
                m_isProfiling = false;
            }
        }

        ImGui::SameLine();

        ImGui::InputDouble("Profiling Duration", &m_profilingDuration, 0.001, 0.100, "%0.3f");

        ImGui::End();
    }
}

void ImGuiLayer::Begin()
{
    BR_PROFILE_FUNCTION();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static bool               dockspaceOpen             = true;
    static bool               opt_fullscreen_persistant = true;
    bool                      opt_fullscreen            = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags           = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and
    // handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become un docked.
    // We cannot preserve the docking relationship between an active window and an inactive docking,
    // otherwise any change of dock space/settings would lead to windows being stuck in limbo and
    // never being visible.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Brigerad DockSpace", &dockspaceOpen, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen) ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("Brigerad DockSpace ID");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
}

void ImGuiLayer::End()
{
    BR_PROFILE_FUNCTION();

    ImGuiIO&     io  = ImGui::GetIO();
    Application& app = Application::Get();
    io.DisplaySize   = ImVec2(float(app.GetWindow().GetWidth()), float(app.GetWindow().GetHeight()));

    // End viewport.
    ImGui::End();

    // Rendering.
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_window = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_window);
    }
}

}    // namespace Brigerad
