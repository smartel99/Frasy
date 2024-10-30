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

#include <format>
#include <imgui.h>

namespace Frasy::Lua {
Popup::Popup(std::size_t uut, sol::table builder)
{
    bool global         = builder["global"];
    m_name              = builder["name"].operator std::string();
    m_routine           = builder["routine"].get<sol::function>();
    m_consumeButtonText = builder["consumeButtonText"].get_or<std::string>("cancel");
    if (!global) { m_name = std::format("UUT{} - {}", uut, m_name); }
    auto elements = builder["elements"].get<std::vector<sol::table>>();
    for (const auto& element : elements) {
        auto kind = element["kind"].get<Element::Kind>();
        switch (kind) {
            case Element::Kind::Text:
                m_elements.push_back(std::make_unique<Text>(element["text"].get<std::string>()));
                break;
            case Element::Kind::Input:
                m_elements.push_back(std::make_unique<Input>(element["text"].get<std::string>(), m_inputs.size()));
                m_inputs.push_back({});
                break;
            case Element::Kind::Button:
                m_elements.push_back(
                  std::make_unique<Button>(element["text"].get<std::string>(), element["action"].get<sol::function>()));
                break;
            case Element::Kind::Image:
                m_elements.push_back(std::make_unique<Image>(element["path"].get<std::string>(),
                                                             element["width"].get<std::size_t>(),
                                                             element["height"].get<std::size_t>()));
                break;
            case Element::Kind::SameLine:
                m_elements.push_back(std::make_unique<SameLine>(element["width"].get<uint32_t>()));
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
    // It's even worse, it block everything. To never be used again!
    ImGui::Begin(m_name.c_str(),
                 nullptr,
                 ImGuiWindowFlags_NoResize |              //
                   ImGuiWindowFlags_NoCollapse |          //
                   ImGuiWindowFlags_NoSavedSettings |     //
                   ImGuiWindowFlags_AlwaysAutoResize |    //
                   ImGuiWindowFlags_NoDocking);

    auto renderText = [](Text* element) { ImGui::Text("%s", element->text.c_str()); };

    auto renderInput = [&](Input* element) {
        if (!element->text.empty()) {
            ImGui::Text("%s", element->text.c_str());
            ImGui::SameLine();
        }
        if (ImGui::InputText("##", element->vBuf, element->vBufLen)) { m_inputs[element->index] = element->vBuf; }
    };

    auto renderButton = [&](Button* element) {
        if (ImGui::Button(element->text.c_str())) {
            std::lock_guard lock {m_luaMutex};
            element->action(m_inputs);
        }
    };

    auto renderImage = [&](Image* element) {
        // Creation must be done in UI thread
        // Otherwise, you get a nice black image
        if (element->texture == nullptr) {
            if (Brigerad::File::CheckIfPathExists(element->path)) {
                element->texture = Brigerad::Texture2D::Create(element->path);
            }
            else {
                const auto placeholderTexture = Brigerad::Texture2D::Create(1, 1);
                uint32_t   magentaTextureData = 0xFFFF00FF;
                placeholderTexture->SetData(&magentaTextureData, sizeof(magentaTextureData));
                BR_CORE_ERROR("Unable to open '{}'!", element->path);
                element->texture = placeholderTexture;
            }
            BR_CORE_DEBUG("Texture ID -> {}", element->texture->getRenderId());
        }
        uint64_t texture = element->texture->getRenderId();
        ImVec2   size    = {element->width != 0 ? static_cast<float>(element->width)
                                                : static_cast<float>(element->texture->GetWidth()),
                       element->height != 0 ? static_cast<float>(element->height)
                                                 : static_cast<float>(element->texture->GetHeight())};
        ImGui::Image(reinterpret_cast<void*>(texture), size);
    };

    auto renderSameLine = [](const SameLine* element) { ImGui::SameLine(element->width); };

    for (std::size_t i = 0; i < m_elements.size(); ++i) {
        auto& element = m_elements[i];
        ImGui::PushID(std::format("Element {}", i).c_str());
        switch (element->kind) {
            case Element::Kind::Text: renderText(static_cast<Text*>(element.get())); break;
            case Element::Kind::Input: renderInput(static_cast<Input*>(element.get())); break;
            case Element::Kind::Button: renderButton(static_cast<Button*>(element.get())); break;
            case Element::Kind::Image: renderImage(static_cast<Image*>(element.get())); break;
            case Element::Kind::SameLine: renderSameLine(static_cast<SameLine*>(element.get())); break;
        }
        ImGui::PopID();
    }
    if (ImGui::Button(m_consumeButtonText.c_str())) { Consume(); }
    ImGui::End();
}

}    // namespace Frasy::Lua
