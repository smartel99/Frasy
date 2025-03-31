/**
 ******************************************************************************
 * @addtogroup version
 * @{
 * @file    version
 * @author  Samuel Martel
 * @brief   Header for the version module.
 *
 * @date 1/30/2020 2:31:05 PM
 *
 ******************************************************************************
 */
#ifndef _version_
#    define _version_

/*****************************************************************************/
/* Includes */
#    include "utils/models/version.h"
#    include <string>

/*****************************************************************************/
/* Exported defines */
#    define STRINGIZE2(s) #s
#    define STRINGIZE(s)  STRINGIZE2(s)

#    define VERSION_MAJOR            5
#    define VERSION_MINOR            4
#    define VERSION_REVISION         0
#    define VERSION_BUILD            1274
#    define VER_FILE_DESCRIPTION_STR "Frasy Universal Test-Bench Control Station"
#    define VER_FILE_VERSION         VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD
#    define VER_FILE_VERSION_STR                                                                                       \
        STRINGIZE(VERSION_MAJOR)                                                                                       \
        "." STRINGIZE(VERSION_MINOR) "." STRINGIZE(VERSION_REVISION) "." STRINGIZE(VERSION_BUILD)

#    define VER_PRODUCTNAME_STR       "Frasy Universal Test-Bench Control Station"
#    define VER_PRODUCT_VERSION       VER_FILE_VERSION
#    define VER_PRODUCT_VERSION_STR   VER_FILE_VERSION_STR
#    define VER_ORIGINAL_FILENAME_STR VER_PRODUCTNAME_STR ".exe"
#    define VER_INTERNAL_NAME_STR     VER_ORIGINAL_FILENAME_STR
#    define VER_AUTHOR_STR            "Written by Paul Thomas and Samuel Martel"
#    define VER_COPYRIGHT_STR                                                                                          \
        R"(This program is free software: you can redistribute it and/or modify it under the\
terms of the GNU General Public License as published by the Free Software Foundation, either\
version 3 of the License, or (at your option) any later version.\
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without\
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU\
General Public License for more details.\
You should have received a copy of the GNU General Public License along with this program. If\
not, see https://www.gnu.org/licenses/>https://www.gnu.org/licenses/.)"

#    ifdef _DEBUG
#        define VER_VER_DEBUG VS_FF_DEBUG
#    else
#        define VER_VER_DEBUG 0
#    endif

#    define VER_FILEOS    VOS_NT_WINDOWS32
#    define VER_FILEFLAGS VER_VER_DEBUG
#    define VER_FILETYPE  VFT_APP

/*****************************************************************************/
/* Exported macro */


/*****************************************************************************/
/* Exported types */
namespace Frasy {
constexpr Version version = {
  .major       = VERSION_MAJOR,
  .minor       = VERSION_MINOR,
  .revision    = VERSION_REVISION,
  .build       = VERSION_BUILD,
  .versionStr  = VER_FILE_VERSION_STR,
  .description = VER_FILE_DESCRIPTION_STR,
  .copyright   = VER_COPYRIGHT_STR,
  .author      = VER_AUTHOR_STR,
};
}

/*****************************************************************************/
/* Exported functions */


/* Have a wonderful day :) */
#endif /* _version_ */
/**
 * @}
 */
/****** END OF FILE ******/
