/**
 ******************************************************************************
 * @addtogroup sliders
 * @{
 * @file    sliders.h
 * @author  Samuel Martel
 * @brief   Header for the sliders module.
 *
 * @date 6/30/2022 10:55:25 AM
 *
 ******************************************************************************
 */
#ifndef sliders_H
#    define sliders_H

/*****************************************************************************/
/* Includes */
#    include <limits>
#    include <string_view>

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
using namespace std::string_view_literals;

/*---------------------------------------------------------------------------*/
/* Sliders with help messages                                                */
/*---------------------------------------------------------------------------*/
bool SliderFloat(std::string_view label,
                 std::string_view helpMessage,
                 float*           v,
                 float            min    = std::numeric_limits<float>::lowest(),
                 float            max    = std::numeric_limits<float>::max(),
                 std::string_view format = "%.3f"sv,
                 float            power  = 1.0f);
bool SliderFloat2(std::string_view label,
                  std::string_view helpMessage,
                  float            v[2],
                  float            min    = std::numeric_limits<float>::lowest(),
                  float            max    = std::numeric_limits<float>::max(),
                  std::string_view format = "%.3f"sv,
                  float            power  = 1.0f);
bool SliderFloat3(std::string_view label,
                  std::string_view helpMessage,
                  float            v[3],
                  float            min    = std::numeric_limits<float>::lowest(),
                  float            max    = std::numeric_limits<float>::max(),
                  std::string_view format = "%.3f"sv,
                  float            power  = 1.0f);
bool SliderFloat4(std::string_view label,
                  std::string_view helpMessage,
                  float            v[4],
                  float            min    = std::numeric_limits<float>::lowest(),
                  float            max    = std::numeric_limits<float>::max(),
                  std::string_view format = "%.3f"sv,
                  float            power  = 1.0f);

bool SliderInt(std::string_view label,
               std::string_view helpMessage,
               int*             v,
               int              min    = std::numeric_limits<int>::min(),
               int              max    = std::numeric_limits<int>::max(),
               std::string_view format = "%d"sv);
bool SliderInt2(std::string_view label,
                std::string_view helpMessage,
                int              v[2],
                int              min    = std::numeric_limits<int>::min(),
                int              max    = std::numeric_limits<int>::max(),
                std::string_view format = "%d"sv);
bool SliderInt3(std::string_view label,
                std::string_view helpMessage,
                int              v[3],
                int              min    = std::numeric_limits<int>::min(),
                int              max    = std::numeric_limits<int>::max(),
                std::string_view format = "%d"sv);
bool SliderInt4(std::string_view label,
                std::string_view helpMessage,
                int              v[4],
                int              min    = std::numeric_limits<int>::min(),
                int              max    = std::numeric_limits<int>::max(),
                std::string_view format = "%d"sv);

bool SliderAngle(std::string_view label,
                 std::string_view helpMessage,
                 float*           v,
                 float            min    = -360.0f,
                 float            max    = 360.0f,
                 std::string_view format = "%.0f deg"sv);

/*---------------------------------------------------------------------------*/
/* Sliders without help messages                                             */
/*---------------------------------------------------------------------------*/
bool SliderFloat(std::string_view label,
                 float*           v,
                 float            min    = std::numeric_limits<float>::lowest(),
                 float            max    = std::numeric_limits<float>::max(),
                 std::string_view format = "%.3f"sv,
                 float            power  = 1.0f);
bool SliderFloat2(std::string_view label,
                  float            v[2],
                  float            min    = std::numeric_limits<float>::lowest(),
                  float            max    = std::numeric_limits<float>::max(),
                  std::string_view format = "%.3f"sv,
                  float            power  = 1.0f);
bool SliderFloat3(std::string_view label,
                  float            v[3],
                  float            min    = std::numeric_limits<float>::lowest(),
                  float            max    = std::numeric_limits<float>::max(),
                  std::string_view format = "%.3f"sv,
                  float            power  = 1.0f);
bool SliderFloat4(std::string_view label,
                  float            v[4],
                  float            min    = std::numeric_limits<float>::lowest(),
                  float            max    = std::numeric_limits<float>::max(),
                  std::string_view format = "%.3f"sv,
                  float            power  = 1.0f);

bool SliderInt(std::string_view label,
               int*             v,
               int              min    = std::numeric_limits<int>::min(),
               int              max    = std::numeric_limits<int>::max(),
               std::string_view format = "%d"sv);
bool SliderInt2(std::string_view label,

                int              v[2],
                int              min    = std::numeric_limits<int>::min(),
                int              max    = std::numeric_limits<int>::max(),
                std::string_view format = "%d"sv);
bool SliderInt3(std::string_view label,

                int              v[3],
                int              min    = std::numeric_limits<int>::min(),
                int              max    = std::numeric_limits<int>::max(),
                std::string_view format = "%d"sv);
bool SliderInt4(std::string_view label,

                int              v[4],
                int              min    = std::numeric_limits<int>::min(),
                int              max    = std::numeric_limits<int>::max(),
                std::string_view format = "%d"sv);

bool SliderAngle(
  std::string_view label, float* v, float min = -360.0f, float max = 360.0f, std::string_view format = "%.0f deg"sv);
}    // namespace UI

/* Have a wonderful day :) */
#endif /* sliders_H */
/**
 * @}
 */
/****** END OF FILE ******/
