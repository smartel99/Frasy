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

target_compile_definitions(frasy_dep_build_options INTERFACE
        $<$<CONFIG:Debug>:BR_DEBUG>
        $<$<CONFIG:Debug>:BR_ENABLE_ASSERTS>
)

if (MSVC)
    target_compile_options(frasy_dep_build_options INTERFACE
            $<$<CONFIG:Debug>:/Od>
            $<$<CONFIG:Release>:/O2 /Zi>
            $<$<CONFIG:RelWithDebInfo>:/O2 /Zi>
            $<$<CONFIG:MinSizeRel>:/Os>
    )
else ()
    target_compile_options(frasy_dep_build_options INTERFACE
            $<$<CONFIG:Debug>:-O0 -g>
            $<$<CONFIG:Release>:-Os>
            $<$<CONFIG:RelWithDebInfo>:-Ofast -g>
            $<$<CONFIG:MinSizeRel>:-Os>
    )
    target_link_options(frasy_dep_build_options INTERFACE
            $<$<CONFIG:Debug>:-O0 -g>
            $<$<CONFIG:Release>:-Os>
            $<$<CONFIG:RelWithDebInfo>:-Ofast -g>
            $<$<CONFIG:MinSizeRel>:-Os>
    )
endif ()

target_link_libraries(frasy_build_options INTERFACE frasy_dep_build_options)
