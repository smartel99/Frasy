cmake_minimum_required(VERSION 3.22)

set(APP_NAME "demo_mode")
project(${APP_NAME}
        LANGUAGES CXX
        VERSION 0.0.1
)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${APP_NAME}")

# Frasy is in the parent directory
set(FRASY_DIR "${CMAKE_SOURCE_DIR}/..")

include(${FRASY_DIR}/compile_options.cmake)

file(GLOB_RECURSE SOURCES
        "${SRC_DIR}/*.cpp"
)

add_subdirectory(${FRASY_DIR} ${CMAKE_BINARY_DIR}/frasy)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
        frasy_build_options
        Frasy
)

add_custom_target(refresh_config ALL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SRC_DIR}/config.json"
        "$<TARGET_FILE_DIR:${APP_NAME}>/config.json"
)

include(${FRASY_DIR}/sync_assets.cmake)
target_sync_assets(refresh_lua_user
        "${SRC_DIR}/lua/user"
        "$<TARGET_FILE_DIR:${APP_NAME}>/lua/user"
)
add_dependencies(refresh_lua_user
        refresh_lua_core
        refresh_config
)
add_dependencies(${PROJECT_NAME} refresh_lua_user)
