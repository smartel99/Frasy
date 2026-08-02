add_executable(FrasyTest_CheckField
    valid_fields.cpp
    valid_fields_extra_args.cpp
    invalid_fields.cpp
    maybe_integration.cpp
    error_messages.cpp
)
target_link_libraries(FrasyTest_CheckField PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_CheckField PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
add_dependencies(FrasyTest_CheckField sync_test_lua)
gtest_discover_tests(FrasyTest_CheckField WORKING_DIRECTORY ${FRASY_TEST_LUA_DIR})
