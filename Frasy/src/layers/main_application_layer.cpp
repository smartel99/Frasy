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

#include <Brigerad/Renderer/asset_manager.h>
#include <frasy_interpreter.h>
#include <imgui.h>
#include <implot.h>
#include <utils/imgui/table.h>

#define CREATE_TEXTURE(texture, name, path)                                                                            \
    do {                                                                                                               \
        texture = Brigerad::AssetManager::AddTexture2D(name, path);                                                    \
    } while (0)

namespace Frasy {
void MainApplicationLayer::onAttach()
{
    BR_PROFILE_FUNCTION();
    // Create textures.
    CREATE_TEXTURE(m_run, "run", "assets/textures/run.png");
    CREATE_TEXTURE(m_runWarn, "runWarn", "assets/textures/run_warn.png");
    CREATE_TEXTURE(m_pass, "pass", "assets/textures/pass.png");
    CREATE_TEXTURE(m_fail, "fail", "assets/textures/fail.png");
    CREATE_TEXTURE(m_error, "error", "assets/textures/error.png");
    CREATE_TEXTURE(m_testing, "testing", "assets/textures/testing.png");
    CREATE_TEXTURE(m_waiting, "waiting", "assets/textures/waiting.png");
    CREATE_TEXTURE(m_idle, "idle", "assets/textures/idle.png");
    CREATE_TEXTURE(m_disabled, "disabled", "assets/textures/disabled.png");
    CREATE_TEXTURE(m_abort, "abort", "assets/textures/emergency_stop.png");

    m_logWindow      = std::make_unique<LogWindow>();
    m_deviceViewer   = std::make_unique<DeviceViewer>(m_canOpen);
    m_canOpenViewer  = std::make_unique<CanOpenViewer::Layer>(m_canOpen);
    m_resultViewer   = std::make_unique<ResultViewer>();
    m_resultAnalyzer = std::make_unique<ResultAnalyzer>();
    m_testViewer     = std::make_unique<TestViewer>();
    m_testViewer->SetInterface(this);

    bool  maximized = Interpreter::Get().getConfig().value("maximized", true);
    auto& window    = Interpreter::Get().getWindow();
    if (maximized) { window.Maximize(); }
    else {
        window.Restore();
    }

    m_logWindow->onAttach();
    m_deviceViewer->onAttach();
    m_canOpenViewer->onAttach();
    m_resultViewer->onAttach();
    m_resultAnalyzer->onAttach();
    m_testViewer->onAttach();
    m_resultAnalyzer->setGetTitle([this] { return m_orchestrator.getTitle(); });

    m_orchestrator.setCanOpen(&m_canOpen);

    m_logWindow->SetVisibility(true);
}


void MainApplicationLayer::onDetach()
{
    BR_PROFILE_FUNCTION();

    m_logWindow->onDetach();
    m_deviceViewer->onDetach();
    m_canOpenViewer->onDetach();
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
    if (Brigerad::Input::isKeyPressed(Brigerad::KeyCode::F7)) { makeCanOpenViewerVisible(); }
    if (Brigerad::Input::isKeyPressed(Brigerad::KeyCode::F8)) { m_renderProfiler = true; }
    m_logWindow->onUpdate(ts);
    m_deviceViewer->onUpdate(ts);
    m_canOpenViewer->onUpdate(ts);
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
            if (ImGui::MenuItem("CANopen Viewer", "F7")) { makeCanOpenViewerVisible(); }
            if (ImGui::MenuItem("Lua Profiler", "F8")) { m_renderProfiler = true; }
            ImGui::Separator();
            if (m_noMove && ImGui::MenuItem("Unlock")) { m_noMove = false; }
            if (!m_noMove && ImGui::MenuItem("Lock")) { m_noMove = true; }
            ImGui::EndMenu();
        }

        appendToMainTabBar();

        if (ImGui::BeginMenu("?")) {
            if (ImGui::MenuItem("About")) { m_renderAbout = true; }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();

        if (m_renderAbout) { renderAbout(); }
        if (m_renderProfiler) { renderProfiler(); }
    }

    PresetControlRoomOptions();
    renderControlRoom();

    m_logWindow->onImGuiRender();
    m_deviceViewer->onImGuiRender();
    m_canOpenViewer->onImGuiRender();
    m_resultViewer->onImGuiRender();
    m_resultAnalyzer->onImGuiRender();
    m_testViewer->onImGuiRender();

    m_orchestrator.renderPopups();
    handleResultAnalyserPopup();
}


void MainApplicationLayer::onEvent(Brigerad::Event& e)
{
    // Creates a dispatch context with the event.
    Brigerad::EventDispatcher dispatcher(e);
    // Dispatch it to the proper handling function in the Application, if the type matches.
    dispatcher.Dispatch<Brigerad::WindowMaximizedEvent>([]([[maybe_unused]] Brigerad::WindowMaximizedEvent& e) {
        Interpreter::Get().getConfig()["maximized"] = true;
        return true;
    });
    dispatcher.Dispatch<Brigerad::WindowRestoredEvent>([]([[maybe_unused]] Brigerad::WindowRestoredEvent& e) {
        Interpreter::Get().getConfig()["maximized"] = false;
        return true;
    });

    m_logWindow->onEvent(e);
    m_deviceViewer->onEvent(e);
    m_canOpenViewer->onEvent(e);
    m_resultViewer->onEvent(e);
    m_resultAnalyzer->onEvent(e);
    m_resultViewer->onEvent(e);
}

void MainApplicationLayer::makeLogWindowVisible()
{
    m_logWindow->SetVisibility(true);
}

void MainApplicationLayer::makeDeviceViewerVisible()
{
    m_deviceViewer->setVisibility(true);
}

void MainApplicationLayer::makeCanOpenViewerVisible()
{
    m_canOpenViewer->setVisibility(true);
}

void MainApplicationLayer::makeResultViewerVisible()
{
    m_resultViewer->setVisibility(true);
}

void MainApplicationLayer::makeResultAnalyzerVisible()
{
    m_resultAnalyzer->setVisibility(true);
}

void MainApplicationLayer::makeTestViewerVisible()
{
    m_testViewer->SetVisibility(true);
}

void MainApplicationLayer::renderAbout()
{
    ImGui::Begin("About", &m_renderAbout);

    ImGui::Text("%s version %s", version.description, version.versionStr);
    ImGui::TextUnformatted(version.author);
    ImGui::TextUnformatted(version.copyright);

    ImGui::End();
}

void MainApplicationLayer::renderProfiler()
{
    if (!ImGui::Begin("Lua Profiler", &m_renderProfiler)) { goto skip_render; }

    if (ImGui::Button("Reset All")) {
        Profiler& profiler = Profiler::get();
        profiler.disable();
        profiler.reset();
        profiler.enable();
        m_profileGraphPopups.clear();
    }

    ImGui::SameLine();
    if (ImGui::Button("Dump Trace")) { DumpProfileEvents(); }

    if (!ImGui::BeginTabBar("profiler_threads")) { return; }
    for (auto&& [id, details] : Profiler::get().getEvents()) {
        renderProfilerTable(id, details);
    }
    ImGui::EndTabBar();

    renderProfilerGraphs();

skip_render:
    ImGui::End();
}

void MainApplicationLayer::renderProfilerTable(const std::thread::id& id, const ProfilerDetails& details)
{
    bool        isTabActive = ImGui::BeginTabItem(details.label().c_str());
    const auto& events      = details.getEvents();
    std::string threadId    = std::format("Thread {}", id);

    float totalTime      = static_cast<float>(details.getTotalTime().count());
    auto  onHoverTooltip = [&threadId, &totalTime, &top = events.front()] {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(800.0f);
        ImGui::Text(threadId.c_str());
        ImGui::Text("Total time: %fus", totalTime);
        ImGui::Text("Top Function: %s", top.header.name.c_str());
        ImGui::Text("Location: %s", top.header.source.c_str());
        ImGui::Text("At line: %d", top.header.currentLine);
        ImGui::PopTextWrapPos();
        ImGui::Separator();
        ImGui::EndTooltip();
    };

    if (ImGui::IsItemHovered() && !events.empty()) { onHoverTooltip(); }
    if (!isTabActive) { return; }

    if (!ImGui::BeginTabBar(details.label().c_str())) {
        ImGui::EndTabItem();    // details.label tab item.
        return;
    }
    isTabActive = ImGui::BeginTabItem(threadId.c_str());
    if (ImGui::IsItemHovered() && !events.empty()) { onHoverTooltip(); }
    if (!isTabActive) {
        ImGui::EndTabItem();    // details.label tab item.
        ImGui::EndTabBar();     // Threads tab bar.
        return;
    }
    if (ImGui::Button(std::format("Reset##{}", id).c_str())) {
        Profiler& profiler = Profiler::get();
        profiler.disable();
        profiler.reset();
        profiler.enable();
        m_profileGraphPopups.clear();
    }
    ImGui::Text("Total time: %fus", totalTime);

    float indent = 0.0f;
    Widget::Table(std::format("profile events##{}", id), 8)
      .ColumnHeader("Name")
      .ColumnHeader("Hit Count")
      .ColumnHeader("Total Time")
      .ColumnHeader("Average Time")
      .ColumnHeader("Source", ImGuiTableColumnFlags_DefaultHide)
      .ColumnHeader("Min Time")
      .ColumnHeader("Max Time")
      .ColumnHeader("Graph")
      .ScrollFreeze()
      .FinishHeader()
      .Content(events, [this, &indent, &totalTime](Widget::Table&, const ProfileEvent& event) -> void {
          renderProfilerTableRow(event, totalTime, indent);
      });
    ImGui::EndTabItem();
    ImGui::EndTabBar();
    ImGui::EndTabItem();
}

void MainApplicationLayer::renderProfilerTableRow(const ProfileEvent& event, float totalTime, float indent)
{
    bool renderChilds = false;
    Widget::Table::CellContent(
      [&renderChilds, &indent](const std::string& name, bool hasChilds) -> void {
          if (indent != 0.0f) { ImGui::Indent(indent); }
          if (!hasChilds) { ImGui::Text(name.c_str()); }
          else {
              const auto& bg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
              ImGui::PushStyleColor(ImGuiCol_Header, bg);
              ImGui::PushStyleColor(ImGuiCol_HeaderActive, bg);
              ImGui::PushStyleColor(ImGuiCol_HeaderHovered, bg);
              ImGui::PushID(static_cast<const void*>(name.c_str()));
              if (ImGui::CollapsingHeader(name.c_str())) { renderChilds = true; }
              ImGui::PopID();
              ImGui::PopStyleColor(3);
          }
          if (ImGui::IsItemHovered()) {
              ImGui::BeginTooltip();
              ImGui::PushTextWrapPos(800.0f);
              ImGui::Text(name.c_str());
              ImGui::PopTextWrapPos();
              ImGui::EndTooltip();
          }
          if (indent != 0.0f) { ImGui::Unindent(indent); }
      },
      event.header.name,
      !event.childs.empty());
    Widget::Table::CellContentTextTooltip("{}", event.hitCount);
    Widget::Table::CellContentTextWrapped(
      "{:0.2}% ({})", (static_cast<float>(event.totalTime.count()) / totalTime) * 100.0f, event.totalTime);
    Widget::Table::CellContentTextWrapped(
      "{:0.2}% ({})", (static_cast<float>(event.avgTime.count()) / totalTime) * 100.0f, event.avgTime);
    Widget::Table::CellContentTextTooltip("{}:{}", event.header.source, event.header.currentLine);
    Widget::Table::CellContentTextWrapped(
      "{:0.2}% ({})", (static_cast<float>(event.minTime.count()) / totalTime) * 100.0f, event.minTime);
    Widget::Table::CellContentTextWrapped(
      "{:0.2}% ({})", (static_cast<float>(event.maxTime.count()) / totalTime) * 100.0f, event.maxTime);

    Widget::Table::CellContent(
      [this](const ProfileEvent* event) {
          std::string graphWindowLabel = std::format("Graph##{}", reinterpret_cast<std::uintptr_t>(event));
          if (ImGui::Button(graphWindowLabel.c_str())) {
              m_profileGraphPopups.emplace_back(std::move(graphWindowLabel), event, true);
          }
      },
      &event);

    if (renderChilds) {
        indent += ImGui::GetStyle().IndentSpacing;
        for (auto&& child : event.childs) {
            renderProfilerTableRow(child, totalTime, indent);
        }
        indent -= ImGui::GetStyle().IndentSpacing;
    }
}

void MainApplicationLayer::renderProfilerGraphs()
{
    for (auto&& event : m_profileGraphPopups) {
        if (ImGui::Begin(event.windowName.c_str(),
                         &event.render,
                         ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking)) {
            ImGui::TextWrapped("%s - %s:%d",
                               event.event->header.name.c_str(),
                               event.event->header.source.c_str(),
                               event.event->header.currentLine);
            ImGui::TextWrapped("Occurrences: %d, Min Time: %dus, Average Time: %dus, Max Time: %dus",
                               event.event->hitCount,
                               event.event->minTime.count(),
                               event.event->avgTime.count(),
                               event.event->maxTime.count());
            ImGui::Checkbox("Display as samples", &event.displayAsSamples);

            if (ImPlot::BeginPlot("History", ImVec2(800.0f, 342.86f))) {
                ImPlot::SetupAxes(event.displayAsSamples ? "Occurrence" : "Timestamp",
                                  "Time (us)",
                                  ImPlotAxisFlags_AutoFit,
                                  ImPlotAxisFlags_AutoFit);

                ImPlot::PlotScatterG(
                  "History",
                  [](int idx, void* data) -> ImPlotPoint {
                      auto& event = *static_cast<ProfileEventInfo*>(data);
                      try {
                          const auto& [start, end, delta] = event.event->history.at(idx);
                          return {event.displayAsSamples
                                    ? static_cast<double>(idx)
                                    : static_cast<double>(
                                        std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch())
                                          .count()) /
                                        1'000,
                                  static_cast<double>(delta.count())};
                      }
                      catch (std::exception&) {
                          return {0, 0};
                      }
                  },
                  &event,
                  static_cast<int>(event.event->history.size()));

                double average = static_cast<double>(event.event->avgTime.count());
                ImPlot::PlotInfLines("Average", &average, 1, ImPlotInfLinesFlags_Horizontal);
                ImPlot::EndPlot();
            }

            ImGui::End();
        }
    }

    std::erase_if(m_profileGraphPopups, [](const ProfileEventInfo& event) { return !event.render; });
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

void MainApplicationLayer::handleResultAnalyserPopup()
{
    if (m_orchestrator.isRunning() == m_wasRunning) { return; }

    m_wasRunning = m_orchestrator.isRunning();

    if (m_wasRunning) {
        m_resultViewer->setVisibility(false);
        return;
    }

    // else
    for (const auto map = m_orchestrator.getMap(); const auto& uut : map.uuts) {
        if (m_orchestrator.getUutState(uut) != UutState::Failed) { continue; }
        m_resultViewer->setVisibility(true);
        break;
    }
}


namespace {

nlohmann::json eventStackTrace(const ProfileEvent& event)
{
    auto j = nlohmann::json {{"name", event.header.name}};
    if (event.parent != nullptr) { j["parent"] = std::format("{}", reinterpret_cast<std::uintptr_t>(event.parent)); }
    return j;
}

void addEventToJson(nlohmann::json& j, const ProfileEvent& event, const std::thread::id& threadId)
{
    auto frameId              = std::format("{}", reinterpret_cast<std::uintptr_t>(&event));
    j["stackFrames"][frameId] = eventStackTrace(event);
    for (auto&& marker : event.history) {
        j["traceEvents"] += nlohmann::json {
          {"name", event.header.name},
          {"ph", "X"},
          {"pid", 0},
          {"tid", threadId._Get_underlying_id()},
          {"sf", frameId},
          {"ts", std::chrono::duration_cast<std::chrono::microseconds>(marker.start.time_since_epoch()).count()},
          {"dur", marker.delta.count()},
        };
    }

    for (auto&& child : event.childs) {
        addEventToJson(j, child, threadId);
    }
}

nlohmann::json profilerMetadata(const ProfilerDetails& details)
{
    // Metadata for this thread.
    return nlohmann::json {
      {"name", "thread_name"},
      {"ph", "M"},
      {"pid", 0},
      {"tid", details.threadId()._Get_underlying_id()},
      {"args", nlohmann::json {"name", details.label()}},
    };
}
}    // namespace

void MainApplicationLayer::DumpProfileEvents()
{
    nlohmann::json j = nlohmann::json {
      {"otherData", nlohmann::json {}},
      {"traceEvents", nlohmann::json::array()},
      {"stackFrames", nlohmann::json {}},
    };
    for (auto&& [id, profiler] : Profiler::get().getEvents()) {
        j["traceEvents"] += profilerMetadata(profiler);
        for (auto&& event : profiler.getEvents()) {
            addEventToJson(j, event, profiler.threadId());
        }
    }
    std::ofstream file = std::ofstream("profile_events.json");
    file << j.dump();
}

void MainApplicationLayer::generate()
{
    //    m_orchestrator->Generate();
}
void MainApplicationLayer::setTestEnable(const std::string& sequence, const std::string& test, bool enable)
{
    m_orchestrator.setTestEnable(sequence, test, enable);
}
void MainApplicationLayer::setSequenceEnable(const std::string& sequence, bool enable)
{
    m_orchestrator.setSequenceEnable(sequence, enable);
}

}    // namespace Frasy
