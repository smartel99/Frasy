/**
 ******************************************************************************
 * @addtogroup StringUtils
 * @{
 * @file    StringUtils
 * @author  Samuel Martel
 * @brief   Header for the StringUtils module.
 *
 * @date 1/3/2020 12:09:56 PM
 *
 ******************************************************************************
 */
#ifndef _StringUtils
#define _StringUtils

/*****************************************************************************/
/* Includes */

#include <string>

namespace Frasy::StringUtils
{
/*****************************************************************************/
/* Exported defines */


/*****************************************************************************/
/* Exported macro */


/*****************************************************************************/
/* Exported types */


/*****************************************************************************/
/* Exported functions */
std::string  ReplaceAll(const std::string& str,
                        const std::string& toReplace,
                        const std::string& replaceBy);
std::string  GetFullNameFromPath(const std::wstring& path);
std::string  GetFullNameFromPath(const std::string& path);
std::string  GetNameFromPath(const std::string& path);
std::string  RemoveNameFromPath(const std::wstring& path);
std::string  RemoveNameFromPath(const std::string& path);
std::string  WStringToString(const std::wstring& src);
std::wstring StringToWString(const std::string& src);


}  // namespace Frasy::StringUtils


/*****************************************************************************/
#endif /* _StringUtils */
/* Have a wonderful day :) */
/**
 * @}
 */
/****** END OF FILE ******/
