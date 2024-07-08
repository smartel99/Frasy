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
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */
#include "my_main_application_layer.h"

#include <Brigerad/Utils/dialogs/error.h>
#include <frasy_interpreter.h>
#include <imgui.h>

#include <filesystem>
#include <regex>


void MyMainApplicationLayer::onAttach()
{
    MainApplicationLayer::onAttach();
    loadProducts();
}

void MyMainApplicationLayer::onDetach()
{
    MainApplicationLayer::onDetach();

    // TODO Save the currently selected product.
}

void MyMainApplicationLayer::onUpdate(Brigerad::Timestep ts)
{
    MainApplicationLayer::onUpdate(ts);
    if (Brigerad::Input::isKeyPressed(Brigerad::KeyCode::F9)) { loadProducts(); }
    if (Brigerad::Input::isKeyPressed(Brigerad::KeyCode::F10)) { m_skipVerification = !m_skipVerification; }
}

void MyMainApplicationLayer::renderControlRoom()
{

    if (m_activeProduct.empty() || m_map.uuts.size() == 0 || m_map.ibs.size() == 0) {
        ImGui::Begin("Control Room");
        if (ImGui::Button("Reload")) { loadProducts(); }
        ImGui::End();
        return;
    }

    if (!ImGui::Begin("ControlRoom")) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginCombo("Product", m_activeProduct.c_str())) {
        for (auto&& [env, testPath, name, modified] : m_products) {
            if (ImGui::Selectable(name.c_str(), name == m_activeProduct)) {
                makeOrchestrator(name, env, testPath);
                Frasy::Config cfg;
                cfg.setField("LastProduct", m_activeProduct);
                Frasy::FrasyInterpreter::Get().getConfig().setField("Demo", cfg);
            }
        }
        ImGui::EndCombo();
    }

    uint64_t texture {};
    if (m_orchestrator.isRunning()) { texture = m_testing->getRenderId(); }
    else if (m_skipVerification) {
        texture = m_runWarn->getRenderId();
    }
    else {
        texture = m_run->getRenderId();
    }

    ImGui::BeginTable("Launcher", 2);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    static const ImVec2 buttonSize = ImVec2 {100.0f, 100.0f};
    static const ImVec2 buttonUv0  = ImVec2 {0.0f, 1.0f};
    static const ImVec2 buttonUv1  = ImVec2 {1.0f, 0.0f};

    if (ImGui::ImageButton(reinterpret_cast<void*>(texture), buttonSize, buttonUv0, buttonUv1)) {
        m_resultViewer->setVisibility(false);    // Close the result viewer while we run the test.
        doTests();
        m_testJustFinished = false;
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) { ImGui::OpenPopup("Stress Test"); }
    if (ImGui::BeginPopup("Stress Test")) {
        ImGui::SliderInt("Repeat Count", &m_repeatCount, 0, 200);
        ImGui::EndPopup();
    }
    ImGui::TableNextColumn();
    ImGui::InputText("Operator", &m_operator[0], s_operatorLength);
    if (ImGui::InputText("Serial Number - Top Left", &m_serialNumberTopLeft[0], serialNumberLength)) {
        m_serialIsDirty = false;
    }
    if (ImGui::InputText("Serial Number - Bottom Right", &m_serialNumberBottomRight[0], serialNumberLength)) {
        m_serialIsDirty = false;
    }

    if (ImGui::Button("Scan serials")) {}
    ImGui::EndTable();


    if (!m_testJustFinished && !m_orchestrator.isRunning()) {
        if (m_repeatCount != 0) {
            m_repeatCount--;
            doTests();
        }
        else {
            // If we just finished the test, check if any UUT have failed.
            m_testJustFinished = true;
            if (std::ranges::any_of(m_map.uuts, [this](const auto& uut) {
                    return m_orchestrator.GetUutState(uut) == Frasy::UutState::Failed;
                })) {
                m_resultViewer->setVisibility(true);
            }
        }
    }

    static constexpr std::size_t          unteamedUutsPerLine = 4;
    std::vector<std::vector<std::size_t>> uutLines            = {};
    if (m_map.teams.size() == 0) {
        auto it = m_map.uuts.begin();
        uutLines.emplace_back();
        while (it != m_map.uuts.end()) {
            if (uutLines.back().size() == unteamedUutsPerLine) { uutLines.emplace_back(); }
            uutLines.back().push_back(*it);
            ++it;
        }
    }
    else {
        for (const auto& team : m_map.teams) {
            uutLines.emplace_back(team);
        }
    }
    ImGui::BeginTable("UUT", unteamedUutsPerLine);
    for (std::size_t i = 0; i < uutLines.size(); ++i) {
        const auto&         line       = uutLines[i];
        static const ImVec2 buttonSize = ImVec2 {80.0f, 80.0f};
        ImGui::TableNextRow();
        for (const auto& uut : line) {
            ImGui::TableNextColumn();
            ImGui::PushID(std::format("UUT{}", uut).c_str());
            ImGui::Text("UUT %zu", uut);
            auto     state = m_orchestrator.GetUutState(uut);
            uint64_t texture {};
            switch (state) {
                case Frasy::UutState::Disabled: texture = m_disabled->getRenderId(); break;
                case Frasy::UutState::Idle: texture = m_idle->getRenderId(); break;
                case Frasy::UutState::Waiting: texture = m_waiting->getRenderId(); break;
                case Frasy::UutState::Running: texture = m_testing->getRenderId(); break;
                case Frasy::UutState::Passed: texture = m_pass->getRenderId(); break;
                case Frasy::UutState::Failed: texture = m_fail->getRenderId(); break;
                case Frasy::UutState::Error: texture = m_error->getRenderId(); break;
            }
            if (ImGui::ImageButton(reinterpret_cast<void*>(texture), buttonSize)) {
                m_orchestrator.ToggleUut(uut);
            }
            ImGui::PopID();
        }
    }
    ImGui::EndTable();
    ImGui::End();
}

void MyMainApplicationLayer::doTests()
{
    if (!getSerials()) { return; }
    bool shouldRegen = shouldRegenerate();
    if (shouldRegen) { BR_LOG_INFO("Frasy", "Regenerating sequences..."); }
    m_orchestrator.RunSolution(m_serials, shouldRegen, m_skipVerification);
}

bool MyMainApplicationLayer::getSerials()
{
    m_serials.clear();
    std::regex  snRe("(.+)([0-9A-F]{3})$");
    std::cmatch matches;
    std::string snPrefix;
    int         snStart {};
    int         snEnd {};

    if (std::regex_search(&m_serialNumberTopLeft[0], matches, snRe)) {
        snPrefix = matches[1];
        snStart  = std::stoi(matches[2], nullptr, 10);
    }
    else {
        Brigerad::warningDialog("Frasy", "Top left serial number is not valid!");
        return false;
    }

    if (std::regex_search(&m_serialNumberBottomRight[0], matches, snRe)) {
        if (snPrefix != matches[1]) {
            Brigerad::warningDialog("Frasy", "The format of the serial numbers do not match!");
            return false;
        }
        snEnd = std::stoi(matches[2], nullptr, 10);
    }
    else {
        Brigerad::warningDialog("Frasy", "Bottom right serial number is not valid!");
        return false;
    }

    const auto& map = m_orchestrator.getMap();
    if (snEnd - snStart != map.uuts.size() - 1) {
        Brigerad::warningDialog("Frasy", "Unable to verify serial numbers. Have you scanned the right UUTs?");
        return false;
    }

    m_serials.reserve(map.uuts.size() + 1);
    m_serials.emplace_back();
    for (int i = snStart; i <= snStart + map.uuts.size() - 1; ++i) {
        m_serials.push_back(std::format("{}{:03x}", snPrefix, i));
    }
    return true;
}

void MyMainApplicationLayer::makeOrchestrator(const std::string& name,
                                              const std::string& envPath,
                                              const std::string& testPath)
{
    {
        if (m_orchestrator.loadUserFiles(envPath, testPath)) {
            m_activeProduct = name;
            m_map           = m_orchestrator.getMap();
            for (const auto& ib : m_map.ibs) {
                m_canOpen.addNode(ib.nodeId);
            }
            m_canOpen.reset();    // CANopen needs to be reloaded on environment changes.
        }
        else {
            Brigerad::warningDialog("Frasy", "Unable to initialize orchestrator!");
            makeLogWindowVisible();
            BR_LOG_ERROR("APP", "Unable to initialize orchestrator!");
            m_map = {};
        }
    }
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
    auto it = std::find_if(
      m_products.begin(), m_products.end(), [&](const auto& item) { return item.name == m_activeProduct; });

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
    Frasy::Config cfg         = Frasy::FrasyInterpreter::Get().getConfig().getField("Demo");
    auto          lastProduct = cfg.getField<std::string>("LastProduct");
    auto          it =
      std::find_if(m_products.begin(), m_products.end(), [&](const auto& item) { return item.name == lastProduct; });
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
