add_executable(FrasyTest_Sdk
    sequence.cpp
    test_func.cpp
    requires.cpp
    expect.cpp
    sync.cpp
    once_exclusive.cpp
    integration.cpp
)
target_link_libraries(FrasyTest_Sdk PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_Sdk PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
add_dependencies(FrasyTest_Sdk sync_test_lua)
gtest_discover_tests(FrasyTest_Sdk WORKING_DIRECTORY ${FRASY_TEST_LUA_DIR})
