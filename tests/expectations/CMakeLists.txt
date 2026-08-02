add_executable(FrasyTest_Expectations
    to_be_true_false.cpp
    to_be_equal.cpp
    to_be_near.cpp
    to_be_in_range.cpp
    to_be_greater_lesser.cpp
    to_be_type_match.cpp
    stage_gating.cpp
)
target_link_libraries(FrasyTest_Expectations PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_Expectations PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
add_dependencies(FrasyTest_Expectations sync_test_lua)
gtest_discover_tests(FrasyTest_Expectations WORKING_DIRECTORY ${FRASY_TEST_LUA_DIR})
