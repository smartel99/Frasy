add_executable(FrasyTest_Utils
    global.cpp
    is.cpp
    maybe.cpp
    bitwise.cpp
    stringize_values.cpp
)
target_link_libraries(FrasyTest_Utils PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_Utils PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
add_dependencies(FrasyTest_Utils sync_test_lua)
gtest_discover_tests(FrasyTest_Utils WORKING_DIRECTORY ${FRASY_TEST_LUA_DIR})
