/**
 ******************************************************************************
 * @addtogroup sliders_misc
 * @{
 * @file    sliders_misc.cpp
 * @author  Samuel Martel
 * @brief   Source for the sliders_misc module.
 *
 * @date 6/30/2022 11:35:08 AM
 *
 ******************************************************************************
 */
/*****************************************************************************/
/* Includes */
#include "../helpers.h"
#include "../sliders.h"

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
bool SliderAngle(const std::string_view label,
                 const std::string_view helpMessage,
                 float*                 v,
                 const float            min,
                 const float            max,
                 const std::string_view format)
{
    return ImGuiWidgetHelper(
      label,
      [&]() -> bool { return ImGui::SliderAngle(std::format("##{}", label).c_str(), v, min, max, format.data()); },
      helpMessage);
}

bool SliderAngle(
  const std::string_view label, float* v, const float min, const float max, const std::string_view format)
{
    return ImGuiWidgetHelper(
      label,
      [&]() -> bool { return ImGui::SliderAngle(std::format("##{}", label).c_str(), v, min, max, format.data()); });
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
