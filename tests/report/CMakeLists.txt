add_executable(FrasyTest_Report
    result.cpp
    expectation_from_table.cpp
    saved_report.cpp
    outcome.cpp
    file_report.cpp
    formatters.cpp
)
target_link_libraries(FrasyTest_Report PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_Report PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
add_dependencies(FrasyTest_Report sync_test_lua)
gtest_discover_tests(FrasyTest_Report WORKING_DIRECTORY ${FRASY_TEST_LUA_DIR})
