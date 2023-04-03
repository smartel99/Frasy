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

namespace Frasy::Lua
{
Popup::Popup(std::size_t uut, sol::table builder)
{
    bool global = builder["global"];
    m_name      = builder["name"];
    m_routine   = builder["routine"];
    if (!global) m_name = std::format("UUT{} - {}", uut, m_name);
    auto elements = builder["elements"].get<std::vector<sol::table>>();
    for (const auto& element : elements)
    {
        auto kind = element["kind"].get<Popup::Element::Kind>();
        auto text = element["value"].get<std::string>();
        switch (kind)
        {
            case Popup::Element::Kind::Text: m_elements.push_back(std::make_unique<Text>(text)); break;
            case Popup::Element::Kind::Input:
                m_elements.push_back(std::make_unique<Input>(text, m_inputs.size()));
                m_inputs.push_back({});
                break;
            case Popup::Element::Kind::Button:
                m_elements.push_back(std::make_unique<Button>(text, element["action"].get<sol::function>()));
                break;
        }
    }
}

std::string Popup::GetName(std::size_t uut, sol::table builder)
{
    bool        global = builder["global"];
    std::string name   = builder["name"];
    if (!global) name = std::format("UUT{} - {}", uut, name);
    return name;
}

void Popup::Routine()
{
    using namespace std::chrono_literals;
    while (!m_consumed)
    {
        if (m_routine)
        {
            std::lock_guard lock {m_luaMutex};
            (*m_routine)(m_inputs);
        }
        else { std::this_thread::sleep_for(10ms); }
    }
}

void Popup::Render()
{
    ImGui::Begin(m_name.c_str(),
                 nullptr,
                 ImGuiWindowFlags_NoResize |              //
                   ImGuiWindowFlags_NoCollapse |          //
                   ImGuiWindowFlags_NoSavedSettings |     //
                   ImGuiWindowFlags_AlwaysAutoResize |    //
                   ImGuiWindowFlags_NoDocking);

    auto renderText = [](Text* element) { ImGui::Text("%s", element->text.c_str()); };

    auto renderInput = [&](Input* element, std::size_t index)
    {
        ImGui::PushID(std::format("Element {}", index).c_str());
        if (!element->text.empty())
        {
            ImGui::Text("%s", element->text.c_str());
            ImGui::SameLine();
        }
        if (ImGui::InputText("##", element->vBuf, element->vBufLen)) { m_inputs[element->index] = element->vBuf; }
        ImGui::PopID();
    };

    auto renderButton = [&](Button* element, std::size_t index)
    {
        ImGui::PushID(std::format("Element {}", index).c_str());
        if (ImGui::Button(element->text.c_str()))
        {
            std::lock_guard lock {m_luaMutex};
            element->action(m_inputs);
        }
        ImGui::PopID();
    };

    for (std::size_t i = 0; i < m_elements.size(); ++i)
    {
        auto& element = m_elements[i];
        switch (element->kind)
        {
            case Element::Kind::Text: renderText(static_cast<Text*>(element.get())); break;
            case Element::Kind::Input: renderInput(static_cast<Input*>(element.get()), i); break;
            case Element::Kind::Button: renderButton(static_cast<Button*>(element.get()), i); break;
        }
    }
    if (ImGui::Button("Cancel")) { Consume(); }
    ImGui::End();
}

}    // namespace Frasy::Lua