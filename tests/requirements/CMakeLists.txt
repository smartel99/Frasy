add_executable(FrasyTest_Requirements
    runtime_requirement.cpp
    scope_requirement_generation.cpp
    scope_requirement_execution.cpp
    scope_requirement_validation.cpp
    order_requirement.cpp
    sync_requirement.cpp
)
target_link_libraries(FrasyTest_Requirements PRIVATE Frasy GTest::gtest_main)
target_include_directories(FrasyTest_Requirements PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
add_dependencies(FrasyTest_Requirements sync_test_lua)
gtest_discover_tests(FrasyTest_Requirements WORKING_DIRECTORY ${FRASY_TEST_LUA_DIR})
