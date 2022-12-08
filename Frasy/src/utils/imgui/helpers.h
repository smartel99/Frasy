/**
 ******************************************************************************
 * @addtogroup helpers
 * @{
 * @file    helpers.h
 * @author  Samuel Martel
 * @brief   Header for the helpers module.
 *
 * @date 6/30/2022 11:11:10 AM
 *
 ******************************************************************************
 */
#ifndef helpers_H
#define helpers_H

/*****************************************************************************/
/* Includes */
#include <string_view>

#include "ImGui/imgui.h"


/*****************************************************************************/
/* Exported Defines and Macros */


/*****************************************************************************/
/* Exported Variables */


/*****************************************************************************/
/* Exported Enums */


/*****************************************************************************/
/* Exported Structs and Classes */
namespace UI
{
template<typename Func>
concept UiFunc = requires(Func func)
{
    {
        func()
    }
    ->std::same_as<bool>;
};

template<UiFunc Func>
constexpr bool ImGuiWidgetHelper(const std::string_view label,
                                 Func                   uiFunc,
                                 const std::string_view helpMessage)
{
    const float  height = ImGui::GetFrameHeightWithSpacing();
    const ImVec2 size   = {0.0f, height};
    ImGui::BeginChildFrame(ImGui::GetID(label.data()), size, ImGuiWindowFlags_NoDecoration);

    ImGui::Columns(2, label.data(), false);
    ImGui::TextUnformatted(label.data());
    ImGui::NextColumn();
    const bool r       = uiFunc();
    const bool hovered = ImGui::IsItemHovered();
    ImGui::Columns(1);

    ImGui::EndChildFrame();
    if (!hovered && ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("%s", helpMessage.data());
    }
    return r;
}

template<UiFunc Func>
constexpr bool ImGuiWidgetHelper(const std::string_view label, Func sliderFunc)
{
    const float  height = ImGui::GetFrameHeightWithSpacing();
    const ImVec2 size   = {0.0f, height};
    ImGui::BeginChildFrame(ImGui::GetID(label.data()), size, ImGuiWindowFlags_NoDecoration);

    ImGui::Columns(2, label.data(), false);
    ImGui::TextUnformatted(label.data());
    ImGui::NextColumn();
    const bool r = sliderFunc();
    ImGui::Columns(1);

    ImGui::EndChildFrame();
    return r;
}
}    // namespace UI

/* Have a wonderful day :) */
#endif /* helpers_H */
/**
 * @}
 */
/****** END OF FILE ******/
