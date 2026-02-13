cmake_minimum_required(VERSION 3.22)

project(GLAD C CXX)

add_library(GLAD
    "${CMAKE_CURRENT_LIST_DIR}/src/glad.c"
)

target_include_directories(GLAD PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)
