add_executable(FrasyTest_Models
    solution.cpp
    generation.cpp
)
target_link_libraries(FrasyTest_Models PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_Models PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
add_dependencies(FrasyTest_Models sync_test_lua)
gtest_discover_tests(FrasyTest_Models WORKING_DIRECTORY ${FRASY_TEST_LUA_DIR})
