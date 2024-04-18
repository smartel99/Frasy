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

#define CREATE_TEXTURE(texture, path)                                                                                  \
    do {                                                                                                               \
        if (Brigerad::File::CheckIfPathExists(path) == true) { texture = Brigerad::Texture2D::Create(path); }          \
        else {                                                                                                         \
            BR_CORE_ERROR("Unable to open '{}'!", path);                                                               \
            texture = placeholderTexture;                                                                              \
        }                                                                                                              \
    } while (0)

namespace Frasy {
void MainApplicationLayer::onAttach()
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
    m_deviceViewer   = std::make_unique<DeviceViewer>(m_device);
    m_resultViewer   = std::make_unique<ResultViewer>();
    m_resultAnalyzer = std::make_unique<ResultAnalyzer>();
    m_testViewer     = std::make_unique<TestViewer>();
    m_testViewer->SetInterface(this);

    m_logWindow->onAttach();
    m_deviceViewer->onAttach();
    m_resultViewer->onAttach();
    m_resultAnalyzer->onAttach();
    m_testViewer->onAttach();
}


void MainApplicationLayer::onDetach()
{
    BR_PROFILE_FUNCTION();

    m_logWindow->onDetach();
    m_deviceViewer->onDetach();
    m_resultViewer->onDetach();
    m_testViewer->onDetach();
    m_resultAnalyzer->onDetach();
}


void MainApplicationLayer::onUpdate(Brigerad::Timestep ts)
{
    BR_PROFILE_FUNCTION();

    if (Brigerad::Input::isKeyPressed(Brigerad::KeyCode::F2)) { makeLogWindowVisible(); }
    if (Brigerad::Input::isKeyPressed(Brigerad::KeyCode::F3)) { makeDeviceViewerVisible(); }
    if (Brigerad::Input::isKeyPressed(Brigerad::KeyCode::F4)) { makeResultViewerVisible(); }
    if (Brigerad::Input::isKeyPressed(Brigerad::KeyCode::F5)) { makeResultAnalyzerVisible(); }
    if (Brigerad::Input::isKeyPressed(Brigerad::KeyCode::F6)) { makeTestViewerVisible(); }
    m_logWindow->onUpdate(ts);
    m_deviceViewer->onUpdate(ts);
    m_resultViewer->onUpdate(ts);
    m_testViewer->onUpdate(ts);
    m_resultAnalyzer->onUpdate(ts);
}


void MainApplicationLayer::onImGuiRender()
{
    BR_PROFILE_FUNCTION();
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            //            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) { Brigerad::Application::Get().close(); }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Logger", "F2")) { makeLogWindowVisible(); }
            if (ImGui::MenuItem("Device Viewer", "F3")) { makeDeviceViewerVisible(); }
            if (ImGui::MenuItem("Result Viewer", "F4")) { makeResultViewerVisible(); }
            if (ImGui::MenuItem("Result Analyzer", "F5")) { makeResultAnalyzerVisible(); }
            if (ImGui::MenuItem("Test Viewer", "F6")) { makeTestViewerVisible(); }
            ImGui::Separator();
            if (m_noMove && ImGui::MenuItem("Unlock")) { m_noMove = false; }
            if (!m_noMove && ImGui::MenuItem("Lock")) { m_noMove = true; }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("?")) {
            if (ImGui::MenuItem("About")) { m_renderAbout = true; }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();

        if (m_renderAbout) { renderAbout(); }
    }

    PresetControlRoomOptions();
    renderControlRoom();

    m_logWindow->onImGuiRender();
    m_deviceViewer->onImGuiRender();
    m_resultViewer->onImGuiRender();
    m_resultAnalyzer->onImGuiRender();
    m_testViewer->onImGuiRender();

    m_orchestrator.RenderPopups();
}


void MainApplicationLayer::OnEvent(Brigerad::Event& e)
{
    m_logWindow->OnEvent(e);
    m_deviceViewer->OnEvent(e);
    m_resultViewer->OnEvent(e);
    m_resultAnalyzer->OnEvent(e);
    m_resultViewer->OnEvent(e);
}

void MainApplicationLayer::renderControlRoom()
{
}

void MainApplicationLayer::makeLogWindowVisible()
{
    m_logWindow->SetVisibility(true);
}

void MainApplicationLayer::makeDeviceViewerVisible()
{
    m_deviceViewer->setVisibility(true);
}

void MainApplicationLayer::makeResultViewerVisible()
{
    m_resultViewer->setVisibility(true);
}

void MainApplicationLayer::makeResultAnalyzerVisible()
{
    m_resultAnalyzer->SetVisibility(true);
}

void MainApplicationLayer::makeTestViewerVisible()
{
    m_testViewer->SetVisibility(true);
}

void MainApplicationLayer::renderAbout()
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
    if (!isInFrontOfBg) {
        ImGui::SetNextWindowFocus();
        isInFrontOfBg = true;
    }
    else {
        flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    }

    auto size = ImGui::GetContentRegionAvail();
    ImGui::SetNextWindowPos(ImGui::GetWindowContentRegionMin(), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetWindowContentRegionMax(), ImGuiCond_Always);

    if (m_noMove) { ImGui::Begin("Control Room", nullptr, flags); }
    else {
        ImGui::Begin("Control Room");
    }
    ImGui::End();
}

void MainApplicationLayer::generate()
{
    //    m_orchestrator->Generate();
}
void MainApplicationLayer::setTestEnable(const std::string& sequence, const std::string& test, bool enable)
{
    m_orchestrator.SetTestEnable(sequence, test, enable);
}
void MainApplicationLayer::setSequenceEnable(const std::string& sequence, bool enable)
{
    m_orchestrator.SetSequenceEnable(sequence, enable);
}

}    // namespace Frasy
