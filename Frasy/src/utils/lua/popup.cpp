/**
 * @file    popup.cpp
 * @author  Paul Thomas
 * @date    3/30/2023
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

#include "popup.h"

#include "Brigerad.h"
#include "Brigerad/Core/File.h"

#include <format>
#include <imgui.h>

namespace Frasy::Lua {
void Popup::Text::render()
{
    ImGui::Text("%s", text.c_str());
}

void Popup::Input::render()
{
    if (!title.empty()) {
        ImGui::Text("%s", title.c_str());
        ImGui::SameLine();
    }
    if (ImGui::InputText("##", buffer.data(), bufferLen)) {
        onChange(std::string {buffer.begin(), buffer.end()}, index);
    }
}

void Popup::Button::render()
{
    if (ImGui::Button(label.c_str(), size)) {
        std::lock_guard lock {*luaMutex};
        auto            state  = sol::state_view(action.lua_state());
        auto            result = action(*inputs);
        if (!result.valid()) {
            sol::error error = result;
            BR_LUA_ERROR(error.what());
        }
        if (consume) { onConsume(); }
    }
}

void Popup::Image::render()
{
    // Creation must be done in UI thread
    // Otherwise, you get a nice black image
    if (texture == nullptr) {
        if (Brigerad::File::CheckIfPathExists(path)) { texture = Brigerad::Texture2D::Create(path); }
        else {
            const auto placeholderTexture = Brigerad::Texture2D::Create(1, 1);
            uint32_t   magentaTextureData = 0xFFFF00FF;
            placeholderTexture->SetData(&magentaTextureData, sizeof(magentaTextureData));
            BR_CORE_ERROR("Unable to open '{}'!", path);
            texture = placeholderTexture;
        }
        BR_CORE_DEBUG("Texture ID -> {}", texture->getRenderId());
        if (size.x == 0.0f) { size.x = static_cast<float>(texture->GetWidth()); }
        if (size.y == 0.0f) { size.y = static_cast<float>(texture->GetHeight()); }
    }
    uint64_t texture = this->texture->getRenderId();
    ImGui::Image(reinterpret_cast<void*>(texture), size);
}

void Popup::BeginHorizontal::render()
{
    ImGui::BeginHorizontal(id, size, align);
}

void Popup::EndHorizontal::render()
{
    ImGui::EndHorizontal();
}

void Popup::BeginVertical::render()
{
    ImGui::BeginVertical(id, size, align);
}

void Popup::EndVertical::render()
{
    ImGui::EndVertical();
}

void Popup::SameLine::render()
{
    ImGui::SameLine(offsetFromStartX, spacing);
}

void Popup::Spring::render()
{
    ImGui::Spring(weight, spacing);
}

Popup::Popup(std::size_t uut, sol::table builder)
{
    bool global               = builder["global"];
    m_name                    = builder["name"].operator std::string();
    m_routine                 = builder["routine"].get<sol::function>();
    m_consumeButtonText       = builder["consumeButtonText"].get_or<std::string>("Cancel");
    std::array<float, 2> size = {};
    if (!global) { m_name = std::format("UUT{} - {}", uut, m_name); }
    for (const auto elements = builder["elements"].get<std::vector<sol::table>>(); const auto& element : elements) {
        switch (element["kind"].get<Element::Kind>()) {
            case Element::Kind::Text:
                m_elements.push_back(std::make_unique<Text>(element["text"].get<std::string>()));
                break;
            case Element::Kind::Input:
                m_elements.push_back(std::make_unique<Input>(
                  element["title"].get<std::string>(),
                  m_inputs.size(),
                  [&](const std::string& value, std::size_t index) { m_inputs[index] = value; }));
                m_inputs.emplace_back();
                break;
            case Element::Kind::Button:
                m_elements.push_back(std::make_unique<Button>(
                  element["label"].get<std::string>(),
                  element["size"].get<std::array<float, 2>>(),
                  element["action"].get<sol::function>(),
                  element["consume"].get<bool>(),
                  [&] { Consume(); },
                  &m_luaMutex,
                  &m_inputs));
                break;
            case Element::Kind::Image:
                size = element["size"].get<std::array<float, 2>>();
                m_elements.push_back(
                  std::make_unique<Image>(element["path"].get<std::string>(), ImVec2 {size[0], size[1]}));
                break;
            case Element::Kind::BeginHorizontal:
                size = element["size"].get<std::array<float, 2>>();
                m_elements.push_back(std::make_unique<BeginHorizontal>(
                  element["id"].get<int>(), ImVec2 {size[0], size[1]}, element["align"].get<float>()));
                break;
            case Element::Kind::EndHorizontal: m_elements.push_back(std::make_unique<EndHorizontal>()); break;
            case Element::Kind::BeginVertical:
                size = element["size"].get<std::array<float, 2>>();
                m_elements.push_back(std::make_unique<BeginVertical>(
                  element["id"].get<int>(), ImVec2 {size[0], size[1]}, element["align"].get<float>()));
                break;
            case Element::Kind::EndVertical: m_elements.push_back(std::make_unique<EndVertical>()); break;
            case Element::Kind::SameLine:
                m_elements.push_back(std::make_unique<SameLine>(element["offsetFromStartX"].get<float>(),
                                                                element["spacing"].get<float>()));
                break;
            case Element::Kind::Spring:
                m_elements.push_back(
                  std::make_unique<Spring>(element["weight"].get<float>(), element["spacing"].get<float>()));
                break;
        }
    }
}

std::string Popup::GetName(std::size_t uut, sol::table builder)
{
    bool        global = builder["global"];
    std::string name   = builder["name"];
    if (!global) { name = std::format("UUT{} - {}", uut, name); }
    return name;
}

void Popup::Routine(bool once)
{
    using namespace std::chrono_literals;
    if (once) { m_consumed = true; }
    do {
        if (m_routine && m_routine != sol::nil) {
            std::lock_guard lock {m_luaMutex};
            auto            result = (*m_routine)(m_inputs);
            if (!result.valid()) {
                sol::error err = result;
                BR_LUA_ERROR(err.what());
            }
        }
        else {
            std::this_thread::sleep_for(10ms);
        }
    } while (!m_consumed);
}

void Popup::Render()
{
    // ImGui::SetNextWindowFocus();    // Note: Forcing focus will prevent user to be able to open the top menu
    // It's even worse, it blocks everything. To never be used again!
    ImGui::Begin(m_name.c_str(),
                 nullptr,
                 ImGuiWindowFlags_NoResize |              //
                   ImGuiWindowFlags_NoCollapse |          //
                   ImGuiWindowFlags_NoSavedSettings |     //
                   ImGuiWindowFlags_AlwaysAutoResize |    //
                   ImGuiWindowFlags_NoDocking);

    bool hasConsumeButton = false;
    for (std::size_t i = 0; i < m_elements.size(); ++i) {
        auto& element = m_elements[i];
        ImGui::PushID(std::format("Element {}", i).c_str());
        element->render();
        if (element->kind == Element::Kind::Button) {
            auto* button = static_cast<Button*>(element.get());
            if (button->consume) { hasConsumeButton = true; }
        }
        ImGui::PopID();
    }

    if (!hasConsumeButton) {
        if (ImGui::Button(m_consumeButtonText.c_str())) { Consume(); }
    }
    ImGui::End();
}

}    // namespace Frasy::Lua
