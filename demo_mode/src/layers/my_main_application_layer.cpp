/**
 * @file    my_main_application_layer.cpp
 * @author  Samuel Martel
 * @date    2022-12-05
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <https://www.gnu.org/licenses/>.
 */
#include "my_main_application_layer.h"

#include <Brigerad/Utils/dialogs/error.h>
#include <frasy_interpreter.h>
#include <imgui.h>

#include "expectations_viewer/expectations_viewer.h"
#include <filesystem>
#include <regex>


void MyMainApplicationLayer::onAttach()
{
    MainApplicationLayer::onAttach();
    loadProducts();
}

void MyMainApplicationLayer::onUpdate(Brigerad::Timestep ts)
{
    MainApplicationLayer::onUpdate(ts);
    if (Brigerad::Input::isKeyPressed(Brigerad::KeyCode::F9)) { loadProducts(); }
    if (Brigerad::Input::isKeyPressed(Brigerad::KeyCode::F10)) { m_skipVerification = !m_skipVerification; }
}

void MyMainApplicationLayer::renderControlRoom()
{
    static auto first = true;
    if (!first) { m_imGuiWindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus; }
    first = false;
    ImGui::SetNextWindowPos(ImGui::GetWindowContentRegionMin(), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImGui::GetWindowContentRegionMax(), ImGuiCond_Always);

    if (!ImGui::Begin("ControlRoom", nullptr, m_imGuiWindowFlags)) {
        ImGui::End();
        return;
    }

    renderReloadAndRefreshLuaButton();

    if (renderProductDropdownMenu()) {
        ImGui::End();
        return;
    }

    if (renderEnvironmentError()) {
        ImGui::End();
        return;
    }

    if (m_activeProduct == "expectation") {
        ImGui::BeginChild("Expectations", ImVec2(800.0f, 300.0f));
        renderExpectations(m_orchestrator.getExpectations(1));
        ImGui::EndChild();
    }

    const auto& [ibs, uuts, teams] = m_orchestrator.getMap();

    if (uuts.size() == 1) {
        renderRunButton();
        ImGui::SameLine();
        renderUutIcon(1);
    }
    else if (!teams.empty()) {
        for (auto team : teams) {
            auto title = std::format("Team {}", team.front());
            ImGui::BeginChild("Team");
            bool isFirst = true;
            for (const auto uut : team) {
                if (isFirst) { isFirst = false; }
                else {
                    ImGui::SameLine();
                }
                renderUutIcon(uut);
            }
        }
        renderRunButton();
    }
    else if (!uuts.empty()) {
        bool isFirst = true;
        for (auto uut : uuts) {
            if (isFirst) { isFirst = false; }
            else {
                ImGui::SameLine();
            }
            renderUutIcon(uut);
        }
        renderRunButton();
    }

    ImGui::End();
}

bool MyMainApplicationLayer::renderEnvironmentError()
{
    if (const auto& [ibs, uuts, teams] = m_orchestrator.getMap(); m_activeProduct.empty() || uuts.empty()) {
        if (uuts.empty()) { ImGui::Text("No UUTs found!"); }
        if (m_products.empty()) { ImGui::Text("No products found!"); }
        if (ImGui::Button("Reload")) { loadProducts(); }
        return true;
    }
    return false;
}

bool MyMainApplicationLayer::renderProductDropdownMenu()
{
    ImGui::Text("Product");
    ImGui::SameLine(s_labelWidth);
    ImGui::SetNextItemWidth(s_inputWidth);
    bool update = false;
    if (ImGui::BeginCombo("##Product", m_activeProduct.c_str())) {
        for (auto&& [env, testPath, name, modified] : m_products) {
            if (ImGui::Selectable(name.c_str(), name == m_activeProduct)) {
                makeOrchestrator(name, env, testPath);
                auto& fi           = Frasy::Interpreter::Get();
                auto& cfg          = fi.getConfig();
                cfg["LastProduct"] = m_activeProduct;
                fi.saveConfig();
                update = true;
            }
        }
        ImGui::EndCombo();
    }
    return update;
}

void MyMainApplicationLayer::renderReloadAndRefreshLuaButton()
{
#ifdef BR_DEBUG
    if (ImGui::Button("DEBUG - Update & Refresh Lua")) {
#    ifdef WIN32
        static const auto cdCmd   = R"(cd ..\..\..)";
        static const auto script  = R"(.\generate_hashes.bat)";
        static const auto filters = "";    // TODO: Change per-project requirements

        const auto generateHashesCommand = std::format("{} && {} {}", cdCmd, script, filters);
        system(generateHashesCommand.c_str());

        system(R"(cd ..\..\.. && ..\vendor\bin\premake\premake5.exe frasy)");
#    endif
        loadProducts();
    }
#endif
}

void MyMainApplicationLayer::renderOperatorField()
{
    ImGui::Text("Operator");
    ImGui::SameLine(s_labelWidth);
    ImGui::SetNextItemWidth(s_inputWidth);
    ImGui::InputText("##Operator", m_operatorField.data(), m_operatorField.size());
}

void MyMainApplicationLayer::renderSerialField(std::size_t uut)
{
    ImGui::Text("Serial Number");
    ImGui::SameLine(s_labelWidth);
    ImGui::SetNextItemWidth(s_inputWidth);
    ImGui::InputText("##SerialNumber", m_serialsFields[uut].data(), m_serialsFields[uut].size());
}

void MyMainApplicationLayer::renderRunButton()
{
    uint64_t texture {};
    if (m_orchestrator.isRunning()) { texture = m_testing->getRenderId(); }
    else if (m_skipVerification) {
        texture = m_runWarn->getRenderId();
    }
    else {
        texture = m_run->getRenderId();
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) { ImGui::OpenPopup("Stress Test"); }
    if (ImGui::BeginPopup("Stress Test")) {
        ImGui::SliderInt("Repeat Count", &m_repeatCount, 0, 200);
        ImGui::EndPopup();
    }

    if (ImGui::ImageButton(reinterpret_cast<void*>(texture), m_buttonSize) && !m_orchestrator.isRunning()) {
        m_resultViewer->setVisibility(false);    // Close the result viewer while we run the test.
        doTests();
    }
}

void MyMainApplicationLayer::renderUutIcon(std::size_t uut)
{
    uint64_t texture {};
    switch (m_orchestrator.getUutState(uut)) {
        case Frasy::UutState::Disabled: texture = m_disabled->getRenderId(); break;
        case Frasy::UutState::Idle: texture = m_idle->getRenderId(); break;
        case Frasy::UutState::Waiting: texture = m_waiting->getRenderId(); break;
        case Frasy::UutState::Running: texture = m_testing->getRenderId(); break;
        case Frasy::UutState::Passed: texture = m_pass->getRenderId(); break;
        case Frasy::UutState::Failed: texture = m_fail->getRenderId(); break;
        case Frasy::UutState::Error: texture = m_error->getRenderId(); break;
    }
    if (m_orchestrator.getMap().uuts.size() > 1) {
        if (ImGui::ImageButton(reinterpret_cast<void*>(texture), m_buttonSize)) { m_orchestrator.toggleUut(uut); }
    }
    else {
        auto style = ImGui::GetStyle().FramePadding;
        auto size  = ImVec2(m_buttonSize.x + style.x * 2, m_buttonSize.y + style.y * 2);
        ImGui::Image(reinterpret_cast<void*>(texture), size);
    }
}

void MyMainApplicationLayer::handleTestRepeat()
{
    if (m_orchestrator.isRunning()) { return; }
    if (m_testJustFinished) { return; }
    if (m_repeatCount == 0) { return; }
    m_repeatCount--;
    doTests();
}

void MyMainApplicationLayer::doTests()
{

    const auto operatorName = std::string(m_operatorField.begin(), m_operatorField.end());
    if (operatorName.empty()) {
        BR_APP_ERROR("Operator name cannot be empty");
        return;
    }
    const auto& [ibs, uuts, teams] = m_orchestrator.getMap();
    for (auto uut = 1; uut <= uuts.size(); ++uut) {
        m_serials[uut] = std::string(m_serialsFields[uut].begin(), m_serialsFields[uut].end());
        if (m_orchestrator.getUutState(uut) != Frasy::UutState::Disabled && m_serials[uut].empty()) {
            BR_APP_ERROR("UUT {} serial number cannot be empty", uut);
            return;
        }
    }
    m_serials[0]     = m_serials[1];
    bool shouldRegen = shouldRegenerate();
    if (shouldRegen) { BR_LOG_INFO("Frasy", "Regenerating sequences..."); }
    m_orchestrator.runSolution(operatorName, m_serials, shouldRegen, m_skipVerification);
}

void MyMainApplicationLayer::makeOrchestrator(const std::string& name,
                                              const std::string& envPath,
                                              const std::string& testPath)
{

    if (m_orchestrator.loadUserFiles(envPath, testPath)) {

        m_canOpen.stop();
        m_canOpen.clearNodes();
        m_activeProduct                = name;
        const auto& [ibs, uuts, teams] = m_orchestrator.getMap();
        m_serials.clear();
        m_serials.resize(uuts.size() + 1, {});
        m_serialsFields.clear();
        m_serialsFields.resize(uuts.size() + 1, {});
        for (const auto& ib : ibs | std::views::values) {
            const auto& [kind, nodeId, name, edsPath, od] = ib;
            m_canOpen.addNode(nodeId, name, edsPath);
            if (const uint16_t period = od["Producer heartbeat time"]["value"].get_or<uint16_t>(0); period != 0) {
                // Make frasy aware of a new heartbeat producer, if this node is such a thing.
                // We add a grace period to the beat's period to compensate for the inevitable variability in
                // timing.
                // m_canOpen.setNodeHeartbeatProdTime(ib.nodeId, static_cast<uint16_t>(10 * period));
            }
            else {
                BR_APP_WARN("Node {} ('{}') is not a heartbeat producer!", nodeId, name);
            }
        }
        m_canOpen.start();    // CANopen needs to be reloaded on environment changes.
        m_orchestrator.setLoadUserFunctions([&](const sol::state_view& lua) { loadLuaFunctions(lua); });
    }
    else {
        Brigerad::warningDialog("Frasy", "Unable to initialize orchestrator!");
        makeLogWindowVisible();
        BR_LOG_ERROR("APP", "Unable to initialize orchestrator!");
    }
}

void MyMainApplicationLayer::loadLuaFunctions(sol::state_view lua)
{
    //..
}

std::vector<MyMainApplicationLayer::ProductInfo> MyMainApplicationLayer::detectProducts()
{
    namespace fs = std::filesystem;
    std::vector<ProductInfo> products;

    // For each directory in the lua/user dir, check if there's an environment.lua.
    // If one is present, use a prettified version of the folder's as a product name.
    try {
        for (const auto& entry : fs::recursive_directory_iterator("lua/user")) {
            if (entry.is_directory()) {
                // Product candidate, check if it contains environment.lua
                auto environment = fs::directory_entry(entry.path() / "environment.lua");
                if (environment.exists()) {
                    // Directory is a product, prettify its name.
                    // TODO: do the prettifying
                    auto envPath = environment.path();
                    envPath.replace_extension();    // Remove the .lua from the path.
                    std::string product = entry.path().filename().string();

                    products.emplace_back(ProductInfo {
                      envPath.string(),
                      entry.path().string(),
                      product,
                      getProductFileModificationTimes(entry.path().string()),
                    });
                }
            }
        }
    }
    catch (fs::filesystem_error& e) {
        BR_LOG_ERROR("Demo", "Unable to identify products: {}", e.what());
    }

    return products;
}

bool MyMainApplicationLayer::shouldRegenerate()
{
    auto it = std::ranges::find_if(m_products, [&](const auto& item) { return item.name == m_activeProduct; });

    auto currentModifiedTimes = getProductFileModificationTimes(it->testPath);
    if (std::ranges::any_of(currentModifiedTimes,
                            [&it](const std::pair<std::string, std::filesystem::file_time_type>& entry) {
                                return entry.second < it->lastModifiedTimes[entry.first];
                            })) {
        // One or more files have been modified.
        it->lastModifiedTimes = currentModifiedTimes;
        return true;
    }
    return false;
}

void MyMainApplicationLayer::loadProducts()
{
    m_products = detectProducts();

    if (m_products.empty()) { Brigerad::FatalErrorDialog("Error", "No products found!"); }

    // Re-select the previously selected products, if it still exists.
    auto lastProduct = Frasy::Interpreter::Get().getConfig().value("LastProduct", "");
    auto it          = std::ranges::find_if(m_products, [&](const auto& item) { return item.name == lastProduct; });
    if (it != m_products.end()) { makeOrchestrator(it->name, it->environmentPath, it->testPath); }
    else {
        const auto& [envPath, testPath, name, modified] = m_products.front();
        makeOrchestrator(name, envPath, testPath);
    }
}

std::map<std::string, std::filesystem::file_time_type> MyMainApplicationLayer::getProductFileModificationTimes(
  const std::string& path)
{
    std::map<std::string, std::filesystem::file_time_type> times = {};

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        times[entry.path().string()] = entry.last_write_time();
    }

    return times;
}