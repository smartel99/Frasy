add_library(frasy_dep_build_options INTERFACE)
add_library(frasy_build_options INTERFACE)

set(CMAKE_CXX_STANDARD 23)

if (MSVC)
    target_compile_options(frasy_build_options INTERFACE
            /W4
    )
    # cmake adds /EHsc automatically to the build flags, we however want /EHa instead.
    #
    string(REPLACE "/EHsc" "/EHa" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    target_compile_options(frasy_dep_build_options INTERFACE
            #            /EHa # might not need to have it, depends on CPPTRACE
            /Zc:preprocessor
            /openmp:experimental
            /wd4505 # unreferenced local function has been removed
            /bigobj
    )
    target_compile_definitions(frasy_dep_build_options INTERFACE
            WINVER=0x0A00
            _WIN32_WINNT=0x0A00
            WIN32_LEAN_AND_MEAN
            NOMINMAX
    )
    target_compile_definitions(frasy_dep_build_options INTERFACE -D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else ()
    target_compile_options(frasy_build_options INTERFACE -Wall -Wextra -pedantic-errors)
    target_compile_options(frasy_build_options INTERFACE
            -Wa,-mbig-obj
    )
endif ()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    if (MSVC)
        target_compile_options(frasy_dep_build_options INTERFACE
                /O2
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
        )
    else ()
        target_compile_options(frasy_dep_build_options INTERFACE
                -Ofast
                -g
        )
        target_link_options(frasy_dep_build_options INTERFACE
                -Ofast
                -g
        )
    endif ()
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    if (MSVC)
        target_compile_options(frasy_dep_build_options INTERFACE
                /Os
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
        )
    else ()
        target_compile_options(frasy_dep_build_options INTERFACE
                -O0
                -g
        )
        target_link_options(frasy_dep_build_options INTERFACE
                -O0
                -g
        )
    endif ()
endif ()

target_link_libraries(frasy_build_options INTERFACE frasy_dep_build_options)
