cmake_minimum_required(VERSION 3.22)

project(entt C CXX)

add_library(entt INTERFACE)
target_compile_definitions(entt INTERFACE -D_SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING)
target_include_directories(entt INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}/include"
)
