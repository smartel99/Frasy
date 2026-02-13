cmake_minimum_required(VERSION 3.22)

project(Brigerad CXX C)

file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/src/Brigerad/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/Platform/Windows/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/Platform/OpenGL/*.cpp"
)

add_subdirectory(vendor/entt)
add_subdirectory(vendor/Glad)
include(vendor/cmake/glfw.cmake)
include(vendor/glm/cmake/glm/glmConfig.cmake)
include(vendor/cmake/gtest.cmake)
include(vendor/cmake/imgui.cmake)
include(vendor/cmake/implot.cmake)
include(vendor/cmake/imspinner.cmake)
include(vendor/cmake/lua.cmake)
add_subdirectory(vendor/pfr)
include(vendor/cmake/serial.cmake)
include(vendor/cmake/sol.cmake)

set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "")
add_subdirectory(vendor/spdlog)
include(vendor/cmake/stb_image.cmake)
include(vendor/cmake/tinyfiledialogs.cmake)
add_subdirectory(vendor/yaml-cpp)

add_library(${PROJECT_NAME} STATIC
        ${SOURCES}
)

include(FetchContent)
FetchContent_Declare(
        cpptrace
        GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
        GIT_TAG v1.0.4 # <HASH or TAG>
)
FetchContent_MakeAvailable(cpptrace)

# Needed for shared library builds on windows:  copy cpptrace.dll to the same directory as the
# executable for your_target
if (WIN32)
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:cpptrace::cpptrace>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif ()

target_link_libraries(${PROJECT_NAME} PUBLIC
        frasy_build_options
        entt
        GLAD
        glfw
        glm::glm
        gtest
        ImGui
        ImPlot
        imspinner
        Lua
        Boost::pfr
        serial
        Sol
        spdlog::spdlog
        stb_image
        tinyfiledialogs
        yaml-cpp::yaml-cpp
        cpptrace::cpptrace
        opengl32.lib
        Winmm.lib
        Dwmapi.lib
)
target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src
)
target_compile_definitions(${PROJECT_NAME} PUBLIC
        -DBR_PLATFORM_WINDOWS
)
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
            /wd4100
            /wd4611 #interaction between '_setjmp' and C++ object destruction is non-portable
            /wd4702 #unreachable code
    )
endif ()
