add_executable(FrasyTest_Headless
    test_mcp_server.cpp
    test_console_popup_parsing.cpp
    test_progress_reporter.cpp
    test_product_discovery.cpp
)
target_link_libraries(FrasyTest_Headless PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_Headless PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
gtest_discover_tests(FrasyTest_Headless WORKING_DIRECTORY ${FRASY_TEST_LUA_DIR})
