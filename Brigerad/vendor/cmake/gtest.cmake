set(GTEST_DIR "${CMAKE_CURRENT_LIST_DIR}/../googletest")

add_library(gtest STATIC
        "${GTEST_DIR}/googletest/src/gtest-all.cc"
)

target_include_directories(gtest PUBLIC "${GTEST_DIR}/googletest/include")
target_include_directories(gtest SYSTEM PRIVATE "${GTEST_DIR}/googletest")
target_link_libraries(gtest PUBLIC
        frasy_dep_build_options
)
