add_executable(FrasyTest_ExpectationsClass
        new.cpp
        methods.cpp
        inverted.cpp
        policy.cpp
        consumed.cpp
        export_as.cpp
        on_error_extra.cpp
)
target_link_libraries(FrasyTest_ExpectationsClass PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_ExpectationsClass PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
add_dependencies(FrasyTest_ExpectationsClass sync_test_lua)
gtest_discover_tests(FrasyTest_ExpectationsClass WORKING_DIRECTORY ${FRASY_TEST_LUA_DIR})
