add_executable(FrasyTest_Orchestrator
    create_sequence.cpp
    create_test.cpp
    scope_queries.cpp
    values.cpp
    results.cpp
    enable_disable.cpp
)
target_link_libraries(FrasyTest_Orchestrator PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_Orchestrator PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
add_dependencies(FrasyTest_Orchestrator sync_test_lua)
gtest_discover_tests(FrasyTest_Orchestrator WORKING_DIRECTORY ${FRASY_TEST_LUA_DIR})
