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

MyMainApplicationLayer::MyMainApplicationLayer() : MainApplicationLayer()
{
}

void MyMainApplicationLayer::OnAttach()
{
    MainApplicationLayer::OnAttach();
}

void MyMainApplicationLayer::OnDetach()
{
    MainApplicationLayer::OnDetach();
}

void MyMainApplicationLayer::RenderControlRoom()
{
    ImGui::Begin("Control Room", nullptr);

    auto buttonFunc = [](const auto& texture, const std::string& loggerName, spdlog::level::level_enum level)
    {
        static const ImVec2 buttonSize = ImVec2 {100.0f, 100.0f};
        static const ImVec2 buttonUv0  = ImVec2 {0.0f, 1.0f};
        static const ImVec2 buttonUv1  = ImVec2 {1.0f, 0.0f};
        if (ImGui::ImageButton(
              reinterpret_cast<void*>(static_cast<uint64_t>(texture->GetRenderID())), buttonSize, buttonUv0, buttonUv1))
        {
            BR_LOG(loggerName, level, "This is a message for {}!", loggerName);
        }
        if (ImGui::IsItemActive()) { BR_LOG(loggerName, level, "This is a message for {}!", loggerName); }
    };

    buttonFunc(m_run, "m_run", spdlog::level::trace);
    buttonFunc(m_pass, "m_pass", spdlog::level::debug);
    buttonFunc(m_fail, "m_fail", spdlog::level::info);
    buttonFunc(m_testing, "m_testing", spdlog::level::warn);
    buttonFunc(m_waiting, "m_waiting", spdlog::level::err);
    buttonFunc(m_disabled, "m_disabled", spdlog::level::critical);

    ImGui::End();
}
