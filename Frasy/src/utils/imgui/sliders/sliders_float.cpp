/**
 ******************************************************************************
 * @addtogroup sliders_float
 * @{
 * @file    sliders_float.cpp
 * @author  Samuel Martel
 * @brief   Source for the sliders_float module.
 *
 * @date 6/30/2022 11:09:48 AM
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

bool SliderFloat(const std::string_view label,
                 const std::string_view helpMessage,
                 float*                 v,
                 const float            min,
                 const float            max,
                 const std::string_view format,
                 const float            power)
{
    return ImGuiWidgetHelper(
      label,
      [&]() -> bool {
          return ImGui::SliderFloat(
            std::format("##{}", label).c_str(), v, min, max, format.data(), power);
      },
      helpMessage);
}

bool SliderFloat(const std::string_view label,
                 float*                 v,
                 const float            min,
                 const float            max,
                 const std::string_view format,
                 const float            power)
{
    return ImGuiWidgetHelper(label, [&]() -> bool {
        return ImGui::SliderFloat(
          std::format("##{}", label).c_str(), v, min, max, format.data(), power);
    });
}

bool SliderFloat2(const std::string_view label,
                  const std::string_view helpMessage,
                  float                  v[2],
                  const float            min,
                  const float            max,
                  const std::string_view format,
                  const float            power)
{
    return ImGuiWidgetHelper(
      label,
      [&]() -> bool {
          return ImGui::SliderFloat2(
            std::format("##{}", label).c_str(), v, min, max, format.data(), power);
      },
      helpMessage);
}

bool SliderFloat2(const std::string_view label,
                  float                  v[2],
                  const float            min,
                  const float            max,
                  const std::string_view format,
                  const float            power)
{
    return ImGuiWidgetHelper(label, [&]() -> bool {
        return ImGui::SliderFloat2(
          std::format("##{}", label).c_str(), v, min, max, format.data(), power);
    });
}

bool SliderFloat3(const std::string_view label,
                  const std::string_view helpMessage,
                  float                  v[3],
                  const float            min,
                  const float            max,
                  const std::string_view format,
                  const float            power)
{
    return ImGuiWidgetHelper(
      label,
      [&]() -> bool {
          return ImGui::SliderFloat3(
            std::format("##{}", label).c_str(), v, min, max, format.data(), power);
      },
      helpMessage);
}

bool SliderFloat3(const std::string_view label,
                  float                  v[3],
                  const float            min,
                  const float            max,
                  const std::string_view format,
                  const float            power)
{
    return ImGuiWidgetHelper(label, [&]() -> bool {
        return ImGui::SliderFloat3(
          std::format("##{}", label).c_str(), v, min, max, format.data(), power);
    });
}

bool SliderFloat4(const std::string_view label,
                  const std::string_view helpMessage,
                  float                  v[4],
                  const float            min,
                  const float            max,
                  const std::string_view format,
                  const float            power)
{
    return ImGuiWidgetHelper(
      label,
      [&]() -> bool {
          return ImGui::SliderFloat4(
            std::format("##{}", label).c_str(), v, min, max, format.data(), power);
      },
      helpMessage);
}

bool SliderFloat4(const std::string_view label,
                  float                  v[4],
                  const float            min,
                  const float            max,
                  const std::string_view format,
                  const float            power)
{
    return ImGuiWidgetHelper(label, [&]() -> bool {
        return ImGui::SliderFloat4(
          std::format("##{}", label).c_str(), v, min, max, format.data(), power);
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
