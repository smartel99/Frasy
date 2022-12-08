/**
 ******************************************************************************
 * @addtogroup checkboxes
 * @{
 * @file    checkboxes.h
 * @author  Samuel Martel
 * @brief   Header for the checkboxes module.
 *
 * @date 6/30/2022 1:05:29 PM
 *
 ******************************************************************************
 */
#ifndef checkboxes_H
#define checkboxes_H

/*****************************************************************************/
/* Includes */
#include <string_view>

namespace UI
{

using namespace std::string_view_literals;
/*****************************************************************************/
/* Exported Defines and Macros */


/*****************************************************************************/
/* Exported Variables */


/*****************************************************************************/
/* Exported Enums */


/*****************************************************************************/
/* Exported Structs and Classes */
bool Checkbox(const std::string_view label, const std::string_view helpMessage, bool* b);
bool Checkbox(const std::string_view label, bool* b);

bool CheckboxFlag(const std::string_view label,
                  const std::string_view helpMessage,
                  unsigned int*          b,
                  unsigned int           mask);
bool CheckboxFlag(const std::string_view label, unsigned int* b, unsigned int mask);

}    // namespace UI
/* Have a wonderful day :) */
#endif /* checkboxes_H */
/**
 * @}
 */
/****** END OF FILE ******/
