add_executable(FrasyTest_OrchestratorSolution test.cpp)
target_link_libraries(FrasyTest_OrchestratorSolution PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_OrchestratorSolution PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
add_dependencies(FrasyTest_OrchestratorSolution sync_test_lua)
# Serial: every generation writes the shared lua/solution.json. Debug only, see test.cpp.
gtest_discover_tests(FrasyTest_OrchestratorSolution WORKING_DIRECTORY ${FRASY_TEST_LUA_DIR} PROPERTIES RUN_SERIAL TRUE)
