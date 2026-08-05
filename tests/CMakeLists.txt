# Frasy Unit Tests
#
# How to enable:
#   cmake -DFRASY_BUILD_TESTS=ON -B build
#
# How to run:
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#   (or run the test executables directly from build/vendor/frasy/tests/<module>/)
#
# How to add a new test module:
#   1. Create a new subdirectory under tests/ (e.g., tests/my_module/)
#   2. Add a CMakeLists.txt and test.cpp in that directory
#   3. Add add_subdirectory(my_module) below
#   4. Follow the pattern of existing test modules for CMakeLists.txt content

include(FetchContent)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Directory where Lua files are synced — used as WORKING_DIRECTORY for test execution
set(FRASY_TEST_LUA_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# Sync Lua core files to the test binary directory so tests can find them at runtime.
# Uses the target_sync_assets function from sync_assets.cmake (included by parent).
include(${CMAKE_CURRENT_LIST_DIR}/../sync_assets.cmake)
target_sync_assets(
        sync_test_lua
        "${CMAKE_CURRENT_LIST_DIR}/../Frasy/lua/core"
        "${CMAKE_CURRENT_BINARY_DIR}/lua/core"
)

# Test modules
add_subdirectory(utils)
add_subdirectory(check_field)
add_subdirectory(try_function)
add_subdirectory(timeout_function)
add_subdirectory(sort_utils)
add_subdirectory(expectations)
add_subdirectory(orchestrator)
add_subdirectory(orchestrator_generate)
add_subdirectory(requirements)
add_subdirectory(expectations_class)
add_subdirectory(sdk)
add_subdirectory(environment)
add_subdirectory(team)
add_subdirectory(cli_args)
