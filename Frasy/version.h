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
#define _version_

/*****************************************************************************/
/* Includes */
#include <string>

/*****************************************************************************/
/* Exported defines */
#define STRINGIZE2(s) #s
#define STRINGIZE(s)  STRINGIZE2(s)

#define VERSION_MAJOR            1
#define VERSION_MINOR            5
#define VERSION_REVISION         5
#define VERSION_BUILD               1273
#define VER_FILE_DESCRIPTION_STR "Frasy Universal Test-Bench Control Station"
#define VER_FILE_VERSION         VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION, VERSION_BUILD
#define VER_FILE_VERSION_STR                                                                       \
    STRINGIZE(VERSION_MAJOR)                                                                       \
    "." STRINGIZE(VERSION_MINOR) "." STRINGIZE(VERSION_REVISION) "." STRINGIZE(VERSION_BUILD)

#define VER_PRODUCTNAME_STR       "Frasy Universal Test-Bench Control Station"
#define VER_PRODUCT_VERSION       VER_FILE_VERSION
#define VER_PRODUCT_VERSION_STR   VER_FILE_VERSION_STR
#define VER_ORIGINAL_FILENAME_STR VER_PRODUCTNAME_STR ".exe"
#define VER_INTERNAL_NAME_STR     VER_ORIGINAL_FILENAME_STR
#define VER_AUTHOR_STR            "Written by Samuel Martel"
#define VER_COPYRIGHT_STR         "Copyright SMT Haute-Technologie (C) 2022"

#ifdef _DEBUG
#define VER_VER_DEBUG VS_FF_DEBUG
#else
#define VER_VER_DEBUG 0
#endif

#define VER_FILEOS    VOS_NT_WINDOWS32
#define VER_FILEFLAGS VER_VER_DEBUG
#define VER_FILETYPE  VFT_APP

/*****************************************************************************/
/* Exported macro */


/*****************************************************************************/
/* Exported types */
struct Version
{
    static constexpr size_t      major       = VERSION_MAJOR;
    static constexpr size_t      minor       = VERSION_MINOR;
    static constexpr size_t      revision    = VERSION_REVISION;
    static constexpr size_t      build       = VERSION_BUILD;
    static constexpr const char* versionStr  = VER_FILE_VERSION_STR;
    static constexpr const char* description = VER_FILE_DESCRIPTION_STR;
    static constexpr const char* copyright   = VER_COPYRIGHT_STR;
    static constexpr const char* author      = VER_AUTHOR_STR;
};

/*****************************************************************************/
/* Exported functions */


/* Have a wonderful day :) */
#endif /* _version_ */
/**
 * @}
 */
/****** END OF FILE ******/
