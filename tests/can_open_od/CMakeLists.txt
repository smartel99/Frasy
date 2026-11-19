add_executable(FrasyTest_CanOpenOd
    od_value_test.cpp
    object_dictionary_test.cpp
    object_dictionary_fields_test.cpp
    object_dictionary_registry_test.cpp
    lua_object_dictionary_test.cpp
)
target_link_libraries(FrasyTest_CanOpenOd PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_CanOpenOd PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
# The EDS and XDD of the Frasy node itself, used as reference files.
target_compile_definitions(FrasyTest_CanOpenOd PRIVATE
    FRASY_TEST_CAN_OPEN_DIR="${CANOPEN_OD_DIR}"
)
add_dependencies(FrasyTest_CanOpenOd sync_test_lua)
gtest_discover_tests(FrasyTest_CanOpenOd WORKING_DIRECTORY ${FRASY_TEST_LUA_DIR})
