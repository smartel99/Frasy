add_library(frasy_dep_build_options INTERFACE)
add_library(frasy_build_options INTERFACE)

set(CMAKE_CXX_STANDARD 23)

if (MSVC)
    target_compile_options(frasy_build_options INTERFACE /W4)
    target_compile_options(frasy_dep_build_options INTERFACE /EHa /Zc:preprocessor)
    target_compile_definitions(frasy_dep_build_options INTERFACE -D_CRT_SECURE_NO_WARNINGS)
else ()
    target_compile_options(frasy_build_options INTERFACE -Wall -Wextra -pedantic-errors)
endif ()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    if (MSVC)
        target_compile_options(frasy_dep_build_options INTERFACE
                /O2
                /MT
        )
    else ()
        target_compile_options(frasy_dep_build_options INTERFACE
                -Os
        )
        target_link_options(frasy_dep_build_options INTERFACE
                -Os
        )
    endif ()
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    if (MSVC)
        target_compile_options(frasy_dep_build_options INTERFACE
                /O2
                /MTd
        )
    else ()
        target_compile_options(frasy_dep_build_options INTERFACE
                -Ofast
        )
        target_link_options(frasy_dep_build_options INTERFACE
                -Ofast
        )
    endif ()
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    if (MSVC)
        target_compile_options(frasy_dep_build_options INTERFACE
                /Os
                /MT
        )
    else ()
        target_compile_options(frasy_dep_build_options INTERFACE
                -Os
        )
        target_link_options(frasy_dep_build_options INTERFACE
                -Os
        )
    endif ()
else ()
    message(STATUS "Minimal optimization, debug info included")
    target_compile_definitions(frasy_dep_build_options INTERFACE
            BR_DEBUG
            BR_ENABLE_ASSERTS
    )
    if (MSVC)
        target_compile_options(frasy_dep_build_options INTERFACE
                /Od
                /MTd
        )
    else ()
        target_compile_options(frasy_dep_build_options INTERFACE
                -O0
        )
        target_link_options(frasy_dep_build_options INTERFACE
                -O0
        )
    endif ()
endif ()

target_link_libraries(frasy_build_options INTERFACE frasy_dep_build_options)
