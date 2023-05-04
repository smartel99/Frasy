/******************************************************************************
 * @addtogroup MainApplicationLayer
 * @{
 * @file    main_application_layer.cpp
 * @author  Samuel Martel
 * @brief   Header for the MainApplicationLayer module.
 *
 * @date 9/23/2020 9:32:55 AM
 *
 ******************************************************************************/
#include "main_application_layer.h"

#include "../../version.h"
#include "log_window.h"
#include "result_analyzer.h"

#include <Brigerad/Core/File.h>
#include <imgui/imgui.h>

#define CREATE_TEXTURE(texture, path)                                                                         \
    do {                                                                                                      \
        if (Brigerad::File::CheckIfPathExists(path) == true) { texture = Brigerad::Texture2D::Create(path); } \
        else                                                                                                  \
        {                                                                                                     \
            BR_CORE_ERROR("Unable to open '{}'!", path);                                                      \
            texture = placeholderTexture;                                                                     \
        }                                                                                                     \
    } while (0)

namespace Frasy
{
void MainApplicationLayer::OnAttach()
{
    BR_PROFILE_FUNCTION();
    // Create a white texture to use if the texture files don't exist.
    const auto placeholderTexture = Brigerad::Texture2D::Create(1, 1);
    uint32_t   magentaTextureData = 0xFFFF00FF;
    placeholderTexture->SetData(&magentaTextureData, sizeof(magentaTextureData));

    // Create textures.
    CREATE_TEXTURE(m_run, "assets/textures/run.png");
    CREATE_TEXTURE(m_runWarn, "assets/textures/run_warn.png");
    CREATE_TEXTURE(m_pass, "assets/textures/pass.png");
    CREATE_TEXTURE(m_fail, "assets/textures/fail.png");
    CREATE_TEXTURE(m_error, "assets/textures/error.png");
    CREATE_TEXTURE(m_testing, "assets/textures/testing.png");
    CREATE_TEXTURE(m_waiting, "assets/textures/waiting.png");
    CREATE_TEXTURE(m_idle, "assets/textures/idle.png");
    CREATE_TEXTURE(m_disabled, "assets/textures/disabled.png");

    m_logWindow      = std::make_unique<LogWindow>();
    m_deviceViewer   = std::make_unique<DeviceViewer>();
    m_resultViewer   = std::make_unique<ResultViewer>();
    m_resultAnalyzer = std::make_unique<ResultAnalyzer>();
    m_testViewer     = std::make_unique<TestViewer>();
    m_orchestrator.SetInterface(m_testViewer.get());
    m_testViewer->SetInterface(this);

    m_logWindow->OnAttach();
    m_deviceViewer->OnAttach();
    m_resultViewer->OnAttach();
    m_resultAnalyzer->OnAttach();
    m_testViewer->OnAttach();
}


void MainApplicationLayer::OnDetach()
{
    BR_PROFILE_FUNCTION();

    m_logWindow->OnDetach();
    m_deviceViewer->OnDetach();
    m_resultViewer->OnDetach();
    m_testViewer->OnDetach();
    m_resultAnalyzer->OnDetach();
}


void MainApplicationLayer::OnUpdate(Brigerad::Timestep ts)
{
    BR_PROFILE_FUNCTION();

    if (Brigerad::Input::IsKeyPressed(Brigerad::KeyCode::F2)) { MakeLogWindowVisible(); }
    if (Brigerad::Input::IsKeyPressed(Brigerad::KeyCode::F3)) { MakeDeviceViewerVisible(); }
    if (Brigerad::Input::IsKeyPressed(Brigerad::KeyCode::F4)) { MakeResultViewerVisible(); }
    if (Brigerad::Input::IsKeyPressed(Brigerad::KeyCode::F5)) { MakeResultAnalyzerVisible(); }
    if (Brigerad::Input::IsKeyPressed(Brigerad::KeyCode::F6)) { MakeTestViewerVisible(); }
    m_logWindow->OnUpdate(ts);
    m_deviceViewer->OnUpdate(ts);
    m_resultViewer->OnUpdate(ts);
    m_testViewer->OnUpdate(ts);
    m_resultAnalyzer->OnUpdate(ts);
}


void MainApplicationLayer::OnImGuiRender()
{
    BR_PROFILE_FUNCTION();
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            //            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) { Brigerad::Application::Get().Close(); }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Logger", "F2")) { MakeLogWindowVisible(); }
            if (ImGui::MenuItem("Device Viewer", "F3")) { MakeDeviceViewerVisible(); }
            if (ImGui::MenuItem("Result Viewer", "F4")) { MakeResultViewerVisible(); }
            if (ImGui::MenuItem("Result Analyzer", "F5")) { MakeResultAnalyzerVisible(); }
            if (ImGui::MenuItem("Test Viewer", "F6")) { MakeDeviceViewerVisible(); }
            ImGui::Separator();
            if (m_noMove && ImGui::MenuItem("Unlock")) { m_noMove = false; }
            if (!m_noMove && ImGui::MenuItem("Lock")) { m_noMove = true; }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("?"))
        {
            if (ImGui::MenuItem("About")) { m_renderAbout = true; }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();

        if (m_renderAbout) { RenderAbout(); }
    }

    PresetControlRoomOptions();
    RenderControlRoom();

    m_logWindow->OnImGuiRender();
    m_deviceViewer->OnImGuiRender();
    m_resultViewer->OnImGuiRender();
    m_resultAnalyzer->OnImGuiRender();
    m_testViewer->OnImGuiRender();
}


void MainApplicationLayer::OnEvent(Brigerad::Event& e)
{
    m_logWindow->OnEvent(e);
    m_deviceViewer->OnEvent(e);
    m_resultViewer->OnEvent(e);
    m_resultAnalyzer->OnEvent(e);
    m_resultViewer->OnEvent(e);
}

void MainApplicationLayer::RenderControlRoom()
{
}

void MainApplicationLayer::MakeLogWindowVisible()
{
    m_logWindow->SetVisibility(true);
}

void MainApplicationLayer::MakeDeviceViewerVisible()
{
    m_deviceViewer->SetVisibility(true);
}

void MainApplicationLayer::MakeResultViewerVisible()
{
    m_resultViewer->SetVisibility(true);
}

void MainApplicationLayer::MakeResultAnalyzerVisible()
{
    m_resultAnalyzer->SetVisibility(true);
}

void MainApplicationLayer::MakeTestViewerVisible()
{
    m_testViewer->SetVisibility(true);
}

void MainApplicationLayer::RenderAbout()
{
    ImGui::Begin("About", &m_renderAbout);

    ImGui::Text("%s version %s", Version::description, Version::versionStr);
    ImGui::TextUnformatted(Version::author);
    ImGui::TextUnformatted(Version::copyright);

    ImGui::End();
}

void MainApplicationLayer::PresetControlRoomOptions()
{
    if (m_noMove) { ImGui::SetNextWindowDockID(ImGui::GetWindowDockID()); }

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |    //
                             ImGuiWindowFlags_NoResize |      //
                             ImGuiWindowFlags_NoMove |        //
                             ImGuiWindowFlags_NoNavFocus |    //
                             ImGuiWindowFlags_NoCollapse;

    // Put in front of main window for first initialization
    static bool isInFrontOfBg = false;
    if (!isInFrontOfBg)
    {
        ImGui::SetNextWindowFocus();
        isInFrontOfBg = true;
    }
    else { flags |= ImGuiWindowFlags_NoBringToFrontOnFocus; }

    auto size = ImGui::GetContentRegionAvail();
    ImGui::SetNextWindowPos(ImGui::GetWindowContentRegionMin(), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetWindowContentRegionMax(), ImGuiCond_Always);

    if (m_noMove) { ImGui::Begin("Control Room", nullptr, flags); }
    else { ImGui::Begin("Control Room"); }
    ImGui::End();
}

void MainApplicationLayer::Generate()
{
//    m_orchestrator->Generate();
}
bool MainApplicationLayer::SetTestEnable(const std::string& sequence, const std::string& test, bool enable)
{
    return m_orchestrator.SetTestEnable(sequence, test, enable);
}
bool MainApplicationLayer::SetSequenceEnable(const std::string& sequence, bool enable)
{
    return m_orchestrator.SetSequenceEnable(sequence, enable);
}

}    // namespace Frasy
