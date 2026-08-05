add_executable(FrasyTest_CliArgs
    test.cpp
)
target_link_libraries(FrasyTest_CliArgs PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_CliArgs PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
gtest_discover_tests(FrasyTest_CliArgs)
