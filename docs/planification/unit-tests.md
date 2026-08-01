# Implementation Plan — Unit Test Infrastructure for Frasy (Lua Core Scripts)

## Problem Statement

The Frasy repository has no CMake-integrated test suite. The existing `Tests/LuaDeserializer/test.cpp` is outdated and built via the legacy premake5 system. We need a proper test infrastructure that validates the Lua core scripts (`Frasy/lua/core/*`) using C++ test executables with sol2 and Google Test.

## Requirements

- Tests live in a `tests/` directory (lowercase) with their own `CMakeLists.txt`
- Included from `vendor/frasy/CMakeLists.txt` behind a `FRASY_BUILD_TESTS` option
- Google Test fetched via FetchContent (pinned version)
- CTest enabled with `gtest_discover_tests()`
- Lua files copied to the build directory at build time via custom commands (no staleness)
- Tests use C++ with sol2: create a Lua state, register C++ mocks for external bindings, load scripts, assert behavior
- A shared test fixture provides common mock setup (SleepFor, Hash, DirList, Stage, Context, etc.)
- Delete the obsolete `Tests/` directory and its contents
- Scope: utils (global, Is, maybe, check_field, bitwise, try_function, timeout_function, stringize_values, ini_parser), framework (sort_utils, expectations/common), and the sort sectionize logic

## Background

- The project is at `F:\dev\frasy-template\vendor\frasy`
- The Lua scripts depend on C++ bindings: `SleepFor`, `Hash`, `DirList`, `SaveAsJson`, `CombineAndBitcast`, `Orchestrator.*`, `__exclusive`, `__once`, `ShowExpectation`
- The expectation system's `common.lua` functions check `Context.info.stage == Stage.execution` — tests must set this global to exercise real logic paths
- The `framework/expectation/module.lua` dispatches based on stage — tests should load execution.lua directly or set stage before requiring
- `sort_utils.lua` is pure logic with no C++ dependencies — ideal first test target
- `Is` module is pure Lua — straightforward to test
- `try_function` and `timeout_function` depend on `SleepFor` (mock as no-op or with time tracking)
- `check_field` uses `debug.getinfo` and `io.open` for error messages — can be tested for pass/fail behavior without worrying about the error message format
- The project uses CMake 3.22+, C++23, MSVC on Windows
- `Frasy` is a static library; tests link against it
- sol2 is available via the `Brigerad` dependency (at `Brigerad/vendor/sol`)
- `target_sync_assets()` macro (robocopy/rsync) defined in `sync_assets.cmake` can be reused
- Build options are in `compile_options.cmake` via `frasy_build_options` and `frasy_dep_build_options`
- The parent template CMakeLists at `F:\dev\frasy-template\CMakeLists.txt` sets `APP_NAME` which is referenced in Frasy's CMakeLists — the test CMake should NOT depend on APP_NAME

## Proposed Directory Structure

```
vendor/frasy/
├── CMakeLists.txt                    (add FRASY_BUILD_TESTS option + add_subdirectory)
├── tests/
│   ├── CMakeLists.txt                (FetchContent gtest, enable_testing, lua sync, add subdirs)
│   ├── lua_test_fixture.h            (shared fixture: sol::state + common mocks + global setup)
│   ├── utils/
│   │   ├── CMakeLists.txt
│   │   └── test.cpp                  (global.lua, Is, maybe, bitwise, stringize_values, ini_parser)
│   ├── check_field/
│   │   ├── CMakeLists.txt
│   │   └── test.cpp
│   ├── try_function/
│   │   ├── CMakeLists.txt
│   │   └── test.cpp
│   ├── timeout_function/
│   │   ├── CMakeLists.txt
│   │   └── test.cpp
│   ├── sort_utils/
│   │   ├── CMakeLists.txt
│   │   └── test.cpp
│   └── expectations/
│       ├── CMakeLists.txt
│       └── test.cpp                  (common.lua expectation functions)
```

## Task Breakdown

### Task 1: Create the test infrastructure and shared fixture

**Objective:** Set up `tests/CMakeLists.txt` with FetchContent for GoogleTest, CTest, Lua file syncing, and the shared `lua_test_fixture.h`. Remove the obsolete `Tests/` directory.

**Implementation guidance:**
- Add `option(FRASY_BUILD_TESTS "Build Frasy unit tests" OFF)` to `vendor/frasy/CMakeLists.txt`
- Add at the end (BEFORE the custom targets that reference APP_NAME): `if(FRASY_BUILD_TESTS) add_subdirectory(tests) endif()`
- Delete the `Tests/` directory entirely (LuaDeserializer test is outdated)
- Create `tests/CMakeLists.txt` that:
  - Calls `include(FetchContent)` and fetches `googletest` at tag `v1.15.2`
  - Sets `gtest_force_shared_crt ON` for MSVC compatibility
  - Calls `enable_testing()` and `include(GoogleTest)`
  - Reuses `target_sync_assets` (already included in parent CMakeLists via `include(sync_assets.cmake)`) to sync `${CMAKE_CURRENT_LIST_DIR}/../Frasy/lua/core/` → `${CMAKE_CURRENT_BINARY_DIR}/lua/core/`
  - Adds subdirectories for each test module
- Create `tests/lua_test_fixture.h`:
  - A gtest fixture class that creates a `sol::state` with base/string/table/math/io/debug libraries
  - Registers common mocks: `SleepFor` (no-op), `Hash` (returns 0), `DirList` (returns empty table), `SaveAsJson` (no-op), `CombineAndBitcast` (returns 0), `ShowExpectation` (no-op)
  - Loads `lua/core/framework/stage.lua` (defines `Stage` global)
  - Loads `lua/core/framework/exception.lua` (defines exception constructors)
  - Loads `lua/core/utils/global.lua` (defines `Print`, `ToString`, `Equals`, `Traverse`, `LineSplit`, `ToInt`)
  - Sets up a minimal `Context` table with `Context.info.stage = Stage.execution`
  - Sets up a minimal `Orchestrator` table with no-op functions (`AddExpectationResult`, `GetScope`, `SetValue`, `GetSequenceScopeRequirement`, `GetTestScopeRequirement`)
  - Sets the working directory context so `require()` paths resolve from the binary output dir
  - NOTE: The fixture must configure `package.path` to include the binary output directory so that `require("lua/core/...")` works

**Test requirements:** CMake configures successfully with `-DFRASY_BUILD_TESTS=ON`, GoogleTest is fetched, no test targets fail to configure.

**Demo:** `cmake -DFRASY_BUILD_TESTS=ON -B build` completes without errors. The `tests/` directory structure is in place.

---

### Task 2: Add utils tests (global.lua, Is, maybe, bitwise, stringize_values)

**Objective:** Test the pure-Lua utility functions that form the foundation of the scripting layer.

**Implementation guidance:**
- Create `tests/utils/CMakeLists.txt` and `tests/utils/test.cpp`
- The CMakeLists.txt should:
  - Define an executable target (e.g., `FrasyTest_Utils`)
  - Link against `Frasy` and `GTest::gtest_main`
  - Call `gtest_discover_tests(FrasyTest_Utils WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..)`
  - Add dependency on the lua sync target
- Test `global.lua` functions (already loaded by fixture):
  - `Equals`: same primitives, different primitives, same tables, different tables (nested), different key counts, different types
  - `ToString`: primitives, tables, nested tables, tables with `__tostring` metamethod
  - `Traverse`: valid chain, broken chain, nil input, empty args
  - `LineSplit`: basic multiline string, empty string, single line
  - `ToInt`: positive rounding, negative rounding, exact integers
- Test `Is` module (load via `require("lua/core/utils/is")`):
  - `Is.Integer`, `Is.Float`, `Is.String`, `Is.Boolean`, `Is.Nil`, `Is.Table`, `Is.Array`, `Is.Function`
  - Range checks: `Is.Integer8`, `Is.Integer16`, `Is.Integer32`, `Is.Unsigned8`, `Is.Unsigned16`, `Is.Unsigned32`
  - `Is.IntegerIn`, `Is.FloatIn`, `Is.UnsignedIn` with boundary values
  - `Is.Not` inversion
  - `Is.InArray` with present and absent values
- Test `maybe` (load via `require("lua/core/utils/maybe")`): nil passes, valid value passes, invalid value fails
- Test `bitwise` (load via `require("lua/core/utils/bitwise")`): `Inject` and `Extract` for various bit positions
- Test `stringize_values` (load via `require("lua/core/utils/stringize_values")`): pack bytes, verify string output

Link against `Frasy` and `GTest::gtest_main`. Use the shared fixture for sol state setup.

**Test requirements:** All assertions pass via `ctest -R Utils`.

**Demo:** `ctest -R Utils --output-on-failure` shows all utility function tests passing.

---

### Task 3: Add check_field tests

**Objective:** Test that `check_field` passes valid values through and throws on invalid ones.

**Implementation guidance:**
- Create `tests/check_field/CMakeLists.txt` and `tests/check_field/test.cpp`
- Load `Is` and `check_field` via require
- Test cases:
  - Valid field: `CheckField(5, Is.Integer)` returns 5
  - Invalid field: `CheckField("hello", Is.Integer)` throws an error
  - With additional predicate args: `CheckField(10, Is.IntegerIn, 0, 100)` passes
  - Boundary: `CheckField(101, Is.IntegerIn, 0, 100)` throws
  - Works with Maybe: `CheckField(nil, Maybe, Is.Integer)` passes (nil is OK for Maybe)
- Use `sol::protected_function` to catch Lua errors and assert on them in C++

**Test requirements:** `ctest -R check_field` passes.

**Demo:** Build and run, demonstrating pass-through and error-throwing behavior.

---

### Task 4: Add try_function tests

**Objective:** Test the retry logic in `try_function.lua`.

**Implementation guidance:**
- Create `tests/try_function/CMakeLists.txt` and `tests/try_function/test.cpp`
- Mock `SleepFor` to track call count and arguments (override the fixture's default no-op)
- Test cases:
  - Function succeeds on first try: returns true, SleepFor not called
  - Function succeeds on 3rd try: returns true, SleepFor called twice (delay between retries, not before first)
  - Function never succeeds (default 3 tries): returns false
  - Function never succeeds with `raiseError = true`: throws error
  - Custom `maxTryCount`: verify correct number of retries
  - Custom `delay`: verify SleepFor receives correct ms value
  - Invalid arguments (non-function, non-table opt): throws CheckField error
- Load `Is`, `check_field`, and `try_function` via require

**Test requirements:** `ctest -R try_function` passes.

**Demo:** All retry scenarios verified, including mock call tracking.

---

### Task 5: Add timeout_function tests

**Objective:** Test the timeout/polling logic in `timeout_function.lua`.

**Implementation guidance:**
- Create `tests/timeout_function/CMakeLists.txt` and `tests/timeout_function/test.cpp`
- Mock `SleepFor` to track calls but not actually sleep
- Set `Context.info.stage = Stage.execution` (needed for the timeout error path)
- Test cases:
  - Routine returns false immediately (condition met): no timeout, no SleepFor calls
  - Routine returns true N times then false: SleepFor called N times
  - Routine always returns true: error thrown ("Timeout") after deadline exceeded
  - Custom `sleep_ms`: verify SleepFor receives correct value
  - Invalid arguments: throws CheckField error
  - Non-execution stage: timeout path returns early instead of erroring

**Test requirements:** `ctest -R timeout_function` passes.

**Demo:** Timeout behavior verified without actual delays.

---

### Task 6: Add sort_utils tests

**Objective:** Test the scope/test sorting and sectionization logic used by the orchestrator.

**Implementation guidance:**
- Create `tests/sort_utils/CMakeLists.txt` and `tests/sort_utils/test.cpp`
- `sort_utils.lua` is nearly pure logic — only depends on `ToString` and `InvalidRequirement` globals (already in fixture)
- Load via `require("lua/core/framework/sort_utils")`
- Test `Sort.SortScopes`:
  - No dependencies: all scopes in one layer
  - Linear chain (A→B→C): produces ordered layers
  - First/last edges: placed correctly
  - Circular dependency: throws InvalidRequirement
  - Invalid last (something depends on it): throws
- Test `Sort.Sectionize`:
  - No sync requirements: single section with all stages
  - Sync requirement splits into sections
  - Multiple syncs: multiple sections
- Test `Sort.CombineSectionized`:
  - Verify combined output matches expected structure for known inputs
  - Use the example from the commented-out `sectionize.lua` test as a reference case
- Test `Sort.HasMetDependencies`:
  - All dependencies met: returns true
  - Unmet dependency: returns false
- Test `Sort.AddEdgeRequirement`:
  - Valid edge: adds to structure
  - Duplicate: no-op or error as expected
  - Conflicting edge: throws

**Test requirements:** `ctest -R sort_utils` passes.

**Demo:** Sort ordering verified against hand-computed expected outputs.

---

### Task 7: Add expectation common function tests

**Objective:** Test the `ExpectTo*` functions in `framework/expectation/common.lua`.

**Implementation guidance:**
- Create `tests/expectations/CMakeLists.txt` and `tests/expectations/test.cpp`
- Must set `Context.info.stage = Stage.execution` (otherwise all functions short-circuit to `{pass = true}`)
- Load `lua/core/framework/expectation/common.lua` via `script_file` (it defines globals, not a module)
- Test each function:
  - `ExpectToBeTrue`: true passes, false fails, non-boolean fails
  - `ExpectToBeFalse`: false passes, true fails, non-boolean fails
  - `ExpectToBeEqual`: same value/type passes, different value fails, different type fails
  - `ExpectToBeNear`: within deviation passes, outside fails, non-number fails
  - `ExpectToBeInRange`: within range passes, boundary values, outside fails
  - `ExpectToBeInPercentage`: within percentage passes, outside fails, verify min/max calculation
  - `ExpectToBeGreater` / `ExpectToBeGreaterOrEqual`: boundary cases
  - `ExpectToBeLesser` / `ExpectToBeLesserOrEqual`: boundary cases
  - `ExpectToBeType`: matching type passes, mismatched fails
  - `ExpectToBeMatch`: matching pattern passes, non-match fails, non-string fails
- For each: verify the returned table has correct `method`, `pass`, `expected`/`min`/`max` fields
- Also test that non-execution stage returns `{pass = true}` for all functions

**Test requirements:** `ctest -R expectations` passes.

**Demo:** Full expectation logic verified, including edge cases and stage-gating.

---

### Task 8: Wire everything together and verify end-to-end

**Objective:** Ensure the full test suite builds, all tests are discovered by CTest, and everything passes cleanly.

**Implementation guidance:**
- Run full build with `-DFRASY_BUILD_TESTS=ON`
- Run `ctest --output-on-failure` — all tests pass
- Verify building without `-DFRASY_BUILD_TESTS=ON` produces no test artifacts
- Verify that editing a Lua file and rebuilding triggers the sync (modify a comment in `is.lua`, rebuild, confirm the copy updates)
- Ensure test executables can be run directly (not just via ctest) for debugging
- Add a brief comment block at the top of `tests/CMakeLists.txt` documenting:
  - How to enable tests (`-DFRASY_BUILD_TESTS=ON`)
  - How to run (`ctest --output-on-failure` or run binary directly)
  - How to add a new test module

**Test requirements:** Clean build in both configurations. Full CTest pass. No staleness.

**Demo:** `cmake --build build && ctest --test-dir build --output-on-failure` shows all test modules discovered and passing. Building without the flag confirms no test targets exist.

---

## Important Implementation Notes

- The main `CMakeLists.txt` at `vendor/frasy/CMakeLists.txt` references `${APP_NAME}` in custom targets (sync_assets, copy DLLs). The test infrastructure must NOT depend on APP_NAME. Place the `if(FRASY_BUILD_TESTS)` block before those APP_NAME-dependent targets, or guard it so tests work standalone.
- For `package.path` in the fixture, set it to something like: `lua["package"]["path"] = luaDir + "/?.lua;" + luaDir + "/?/init.lua"` where `luaDir` is the directory where lua files are synced.
- The working directory for `gtest_discover_tests` should be the `tests/` binary directory (where lua/core/ is synced to).
- Each test subdirectory's CMakeLists.txt should be minimal: define executable, link libraries, call gtest_discover_tests, add dependency on lua sync target.
