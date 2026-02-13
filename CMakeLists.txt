cmake_minimum_required(VERSION 3.22)

project(Frasy C CXX)

add_subdirectory(Brigerad)

file(GLOB_RECURSE FRASY_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/Frasy/src/*.cpp"
)

set(CANOPEN_APP_DIR "${CMAKE_CURRENT_LIST_DIR}/Frasy/src/platform/windows/can_open")
set(CANOPEN_OD_DIR "${CMAKE_CURRENT_LIST_DIR}/Frasy/src/utils/communication/can_open")
include(Frasy/vendor/cmake/canopennode.cmake)
include(Frasy/vendor/cmake/json.cmake)
add_subdirectory(Frasy/vendor/hashdir)

add_library(${PROJECT_NAME} STATIC ${FRASY_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC
        frasy_build_options
        Brigerad
        CanOpen
        libhashdir
        json
        wkhtmltox
        Ws2_32.lib
)
target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/Frasy/src
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_LIST_DIR}/Frasy/lua"
        "$<TARGET_FILE_DIR:${APP_NAME}>/lua"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_LIST_DIR}/Frasy/assets"
        "$<TARGET_FILE_DIR:${APP_NAME}>/assets"
)
