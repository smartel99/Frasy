/**
 ******************************************************************************
 * @addtogroup checkboxes
 * @{
 * @file    checkboxes.cpp
 * @author  Samuel Martel
 * @brief   Source for the checkboxes module.
 *
 * @date 6/30/2022 1:10:30 PM
 *
 ******************************************************************************
 */
/*****************************************************************************/
/* Includes */
#include "../checkboxes.h"

#include "../helpers.h"

#include <format>

namespace UI
{
/*****************************************************************************/
/* Macro Definitions */

/*****************************************************************************/
/* Template Definitions */
template<UiFunc Func>
constexpr bool CheckboxHelper(Func func)
{
    ImGui::Columns(2, nullptr, false);
    const bool r = func();
    ImGui::Columns(1);
    return r;
}

/*****************************************************************************/
/* Static Function Declarations */

/*****************************************************************************/
/* Static Variable Declarations */

/*****************************************************************************/
/* Static Member Definitions */

/*****************************************************************************/
/* Public Method Definitions */
bool Checkbox(const std::string_view label, const std::string_view helpMessage, bool* b)
{
    return CheckboxHelper([&]() {
        return ImGuiWidgetHelper(
          label,
          [&]() { return ImGui::Checkbox(std::format("##{}", label).c_str(), b); },
          helpMessage);
    });
}

bool Checkbox(const std::string_view label, bool* b)
{
    return CheckboxHelper([&]() {
        return ImGuiWidgetHelper(
          label, [&]() { return ImGui::Checkbox(std::format("##{}", label).c_str(), b); });
    });
}

bool CheckboxFlag(const std::string_view label,
                  const std::string_view helpMessage,
                  unsigned int*          b,
                  const unsigned int     mask)
{
    return CheckboxHelper([&]() {
        return ImGuiWidgetHelper(
          label,
          [&]() { return ImGui::CheckboxFlags(std::format("##{}", label).c_str(), b, mask); },
          helpMessage);
    });
}

bool CheckboxFlag(const std::string_view label, unsigned int* b, const unsigned int mask)
{
    return CheckboxHelper([&]() {
        return ImGuiWidgetHelper(label, [&]() {
            return ImGui::CheckboxFlags(std::format("##{}", label).c_str(), b, mask);
        });
    });
}
/*****************************************************************************/
/* Private Member Definitions */

/*****************************************************************************/
/* Static Function Definitions */

}    // namespace UI
/* Have a wonderful day :) */
/**
 * @}
 */
/****** END OF FILE ******/
