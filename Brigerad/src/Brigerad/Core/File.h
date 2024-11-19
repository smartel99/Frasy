/**
 ******************************************************************************
 * @addtogroup File
 * @{
 * @file    File.h
 * @author  Samuel Martel
 * @brief   Header for the File module.
 *
 * @date 2/2/2021 1:34:03 PM
 *
 ******************************************************************************
 */
#ifndef BRIGERAD_FILE
#    define BRIGERAD_FILE

/*****************************************************************************/
/* Includes */
#    include <string>
#    include <string_view>

/*****************************************************************************/
/* Exported defines */


/*****************************************************************************/
/* Exported macro */


/*****************************************************************************/
/* Exported types */
namespace Brigerad {
class File {
public:
    static bool CheckIfPathExists(std::string_view path);
    static bool CreateDir(const std::string& path);
};
}    // namespace Brigerad


/*****************************************************************************/
/* Exported functions */


/* Have a wonderful day :) */
#endif /* BRIGERAD_FILE */
/**
 * @}
 */
/****** END OF FILE ******/
