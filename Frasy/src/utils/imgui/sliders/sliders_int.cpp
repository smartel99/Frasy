/**
 ******************************************************************************
 * @addtogroup sliders_int
 * @{
 * @file    sliders_int.cpp
 * @author  Samuel Martel
 * @brief   Source for the sliders_int module.
 *
 * @date 6/30/2022 11:23:10 AM
 *
 ******************************************************************************
 */
/*****************************************************************************/
/* Includes */
#include "../sliders.h"
#include "../helpers.h"

#include <format>

namespace UI
{
/*****************************************************************************/
/* Macro Definitions */

/*****************************************************************************/
/* Template Definitions */

/*****************************************************************************/
/* Static Function Declarations */

/*****************************************************************************/
/* Static Variable Declarations */

/*****************************************************************************/
/* Static Member Definitions */

/*****************************************************************************/
/* Public Method Definitions */
bool SliderInt(const std::string_view label,
               const std::string_view helpMessage,
               int*                   v,
               const int              min,
               const int              max,
               const std::string_view format)
{
    return ImGuiWidgetHelper(
      label,
      [&]() -> bool {
          return ImGui::SliderInt(std::format("##{}", label).c_str(), v, min, max, format.data());
      },
      helpMessage);
}

bool SliderInt2(const std::string_view label,
                const std::string_view helpMessage,
                int                    v[2],
                const int              min,
                const int              max,
                const std::string_view format)
{
    return ImGuiWidgetHelper(
      label,
      [&]() -> bool {
          return ImGui::SliderInt2(std::format("##{}", label).c_str(), v, min, max, format.data());
      },
      helpMessage);
}

bool SliderInt3(const std::string_view label,
                const std::string_view helpMessage,
                int                    v[3],
                const int              min,
                const int              max,
                const std::string_view format)
{
    return ImGuiWidgetHelper(
      label,
      [&]() -> bool {
          return ImGui::SliderInt3(std::format("##{}", label).c_str(), v, min, max, format.data());
      },
      helpMessage);
}

bool SliderInt4(const std::string_view label,
                const std::string_view helpMessage,
                int                    v[4],
                const int              min,
                const int              max,
                const std::string_view format)
{
    return ImGuiWidgetHelper(
      label,
      [&]() -> bool {
          return ImGui::SliderInt4(std::format("##{}", label).c_str(), v, min, max, format.data());
      },
      helpMessage);
}


bool SliderInt(
  const std::string_view label, int* v, const int min, const int max, const std::string_view format)
{
    return ImGuiWidgetHelper(label, [&]() -> bool {
        return ImGui::SliderInt(std::format("##{}", label).c_str(), v, min, max, format.data());
    });
}

bool SliderInt2(const std::string_view label,
                int                    v[2],
                const int              min,
                const int              max,
                const std::string_view format)
{
    return ImGuiWidgetHelper(label, [&]() -> bool {
        return ImGui::SliderInt2(std::format("##{}", label).c_str(), v, min, max, format.data());
    });
}

bool SliderInt3(const std::string_view label,
                int                    v[3],
                const int              min,
                const int              max,
                const std::string_view format)
{
    return ImGuiWidgetHelper(label, [&]() -> bool {
        return ImGui::SliderInt3(std::format("##{}", label).c_str(), v, min, max, format.data());
    });
}

bool SliderInt4(const std::string_view label,
                int                    v[4],
                const int              min,
                const int              max,
                const std::string_view format)
{
    return ImGuiWidgetHelper(label, [&]() -> bool {
        return ImGui::SliderInt4(std::format("##{}", label).c_str(), v, min, max, format.data());
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
