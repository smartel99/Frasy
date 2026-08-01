# Implementation Plan — Framework Unit Tests for Frasy

## Problem Statement

The framework layer (orchestrator, scopes, requirements, expectations, SDK, environment, teams) is the central engine that every Frasy test script interacts with. It has complex state management, multi-stage execution (generation → validation → execution), and several error conditions that need automated testing.

## Requirements

- Derived test fixture (`orchestrator_test_fixture.h`) with orchestrator-specific mocks (Log, Team, profiling, enable_list)
- Unit tests for individual orchestrator functions in isolation
- End-to-end tests that define sequences via the SDK, generate a solution, and verify the output
- Real orchestrator scope for expectation tests (foundation for future report generation tests)
- Tests for the requirement system (RuntimeRequirement, ScopeRequirement in all stages)
- Tests for the environment setup API (UUT, values, teams, workers, versioning)
- Tests for the Team module (leadership, sync, validation)

## Proposed Directory Structure

```
tests/
├── lua_test_fixture.h                  (existing — add Log mock)
├── orchestrator_test_fixture.h         (new — derived fixture with full orchestrator mocks)
├── orchestrator/
│   ├── CMakeLists.txt
│   └── test.cpp                        (scope management, state helpers)
├── orchestrator_generate/
│   ├── CMakeLists.txt
│   └── test.cpp                        (end-to-end Generate() tests)
├── requirements/
│   ├── CMakeLists.txt
│   └── test.cpp                        (RuntimeRequirement, ScopeRequirement, OrderRequirement)
├── expectations_class/
│   ├── CMakeLists.txt
│   └── test.cpp                        (Expectation class with real orchestrator scope)
├── sdk/
│   ├── CMakeLists.txt
│   └── test.cpp                        (Sequence(), Test(), Requires(), Expect(), Sync())
├── environment/
│   ├── CMakeLists.txt
│   └── test.cpp                        (Environment setup API)
├── team/
│   ├── CMakeLists.txt
│   └── test.cpp                        (Team module)
```

## Task Breakdown

### Task 1: Create the orchestrator test fixture

**Objective:** Create `orchestrator_test_fixture.h` that extends `LuaTestFixture` with the full mock environment needed to run orchestrator code. Add `Log` mock to the base fixture since many scripts use it.

**Implementation guidance:**
- Add `Log` table with `.I`, `.D`, `.E`, `.W` to `lua_test_fixture.h` (called by exception.lua, sort_utils, orchestrator)
- Create `orchestrator_test_fixture.h` that:
  - Inherits from `LuaTestFixture`
  - Registers `Team` mock with `.Fail()`, `.Sync()` (no-ops or tracked)
  - Registers `__profileStartEvent`, `__profileEndEvent` as no-ops
  - Loads the orchestrator module (`lua/core/framework/orchestrator.lua`) — this sets up the global `Orchestrator` table and loads its dependencies (Scope, Sequence, Test, ScopeRequirement, Sort, etc.)
  - Loads the SDK (`lua/core/sdk/test.lua`) — provides the user-facing `Sequence()`, `Test()`, `Requires()`, `Expect()`, `Sync()`
  - Sets `Context.info.stage` to `Stage.generation` by default (the stage where sequences/tests are defined)
  - Provides helper methods: `setStage(stage)`, `resetOrchestrator()` (clears all sequences/state for test isolation)
  - Sets up `Context.orchestrator.enable_list` as an empty table

**Test requirements:** Fixture compiles, a trivial test using it passes.

**Demo:** A placeholder test creates a sequence and verifies it was registered.

---

### Task 2: Add orchestrator unit tests (scope management)

**Objective:** Test the orchestrator's scope management functions in isolation.

**Implementation guidance:**
- Test `CreateSequence`:
  - Creates a sequence, verify it exists via `HasSequence`
  - Duplicate name throws `AlreadyDefined`
  - Calling while already in a sequence throws `NestedScope`
- Test `CreateTest`:
  - Creates a test within a sequence scope
  - Calling outside a sequence throws `BadScope`
  - Calling while in a test throws `NestedScope`
- Test `IsInSequence` / `IsInTest`:
  - Returns false initially
  - Returns true after scope is set
  - `IsInTest` requires both sequence and test in scope
- Test `HasSequence` / `HasTest` / `HasScope`:
  - Returns false for non-existent
  - Returns true after creation
- Test `SetValue` / `GetValue` / `HasValue`:
  - Set and retrieve a value
  - Duplicate set throws `AlreadyDefined` (except in generation stage)
  - Get on non-existent throws `NotFound`
- Test `HasPassed` / `HasBeenSkipped`:
  - Returns correct status based on result state
  - Throws `NotFound` for unknown scope
- Test `Enable` / `Disable`:
  - Enable/disable a sequence
  - Enable/disable a test within a sequence
  - Unknown scope throws `BadScope`

**Test requirements:** `ctest -R orchestrator` passes.

**Demo:** Full scope lifecycle tested — creation, querying, state transitions, error conditions.

---

### Task 3: Add orchestrator end-to-end Generate() tests

**Objective:** Test the full generation pipeline — define sequences and tests with requirements, call `Generate()`, verify the produced solution.

**Implementation guidance:**
- Set stage to `Stage.generation` before defining sequences
- Test cases:
  - Simple case: one sequence with two tests, no requirements → solution contains them
  - Ordering: `Requires(Sequence("A"):ToPass())` in sequence B → A before B in solution
  - Test ordering within a sequence: `Requires(Test("T1"):ToPass())` in T2 → T1 before T2
  - First/last: `Requires(Sequence():ToBeFirst())` → that sequence is first in solution
  - Sync requirements: `Sync()` call splits solution into sections
  - Error: no sequences defined → throws `GenerationError`
  - Error: circular dependency between sequences → throws (via Sort)
  - Forward reference: sequence B requires A, but B is defined first → generation retries and succeeds
- After `Generate()`, inspect `Context.orchestrator.solution` structure to verify correct ordering

**Test requirements:** `ctest -R orchestrator_generate` passes.

**Demo:** Complex multi-sequence scenarios with requirements produce correctly ordered solutions.

---

### Task 4: Add requirements tests

**Objective:** Test `RuntimeRequirement`, `ScopeRequirement` (all three stage variants), `OrderRequirement`, and `SyncRequirement`.

**Implementation guidance:**
- `RuntimeRequirement`:
  - `:IsMet()` with function returning true → true
  - `:IsMet()` with function returning false → false
  - `:IsMet()` with function returning non-boolean → throws `InvalidRequirement`
  - Custom reason accessible
- `ScopeRequirement` (generation stage):
  - `:ToBeFirst()` registers an OrderRequirement with kind=first
  - `:ToBeLast()` registers kind=last
  - `:ToPass()` registers kind=after (current scope depends on target)
  - `:ToBeAfter()` registers correctly
  - `:Test(name)` sets the test field on the scope
  - `:HasPassed()` returns true in generation (optimistic)
- `ScopeRequirement` (execution stage):
  - `:ToPass()` returns a RuntimeRequirement that checks `HasPassed`
  - `:ToFail()` returns a RuntimeRequirement that checks not passed
  - `:ToBeComplete()` checks not skipped
  - `:HasPassed()` / `:HasFailed()` reflect actual results
  - `:Value(name)` retrieves exported values
  - Order methods (`:ToBeFirst`, etc.) return always-true requirements (ordering already resolved)
- `ScopeRequirement` (validation stage):
  - All methods return always-true requirements (validation doesn't enforce)
- `OrderRequirement`:
  - `:New()` stores scope, reference, and kind
  - `__tostring` produces readable output
- `SyncRequirement`:
  - `:New()` creates with global kind
  - `:Ib()` changes kind to ib
  - `:IsMet()` always returns true

**Test requirements:** `ctest -R requirements` passes.

**Demo:** All requirement types verified across all three stages.

---

### Task 5: Add Expectation class tests

**Objective:** Test the full `Expectation` class (`execution.lua`) with a real orchestrator scope, verifying expectations are stored correctly.

**Implementation guidance:**
- Set up: create a sequence + test, set scope to that test, set stage to execution
- Test `Expectation:New(value, name)`:
  - Creates expectation with correct value and name
- Test expectation methods (`:ToBeTrue()`, `:ToBeEqual()`, etc.):
  - Verify result is stored in `Context.orchestrator.sequences[seq].tests[test].expectations`
  - Verify `.pass`, `.method`, `.expected` fields
- Test `:Not()`:
  - Inverts the result (pass becomes fail and vice versa)
  - `result.inverted` is set to true
- Test `:Mandatory()`:
  - When pass == inverted (i.e., failed after inversion), throws `UnmetExpectation`
  - When passing, no error
- Test `:ExportAs(name)`:
  - Stores the value in orchestrator values for the current scope
- Test `:OnErrorExtra(extra)`:
  - When expectation fails, extra data is appended to `result.extra`
  - When expectation passes, extra is not added
- Test chaining:
  - `Expect(v, "name"):Not():ToBeEqual(x):Mandatory()` — verify all effects

**Test requirements:** `ctest -R expectations_class` passes.

**Demo:** Expectations are created, stored, inverted, enforced, and export values correctly.

---

### Task 6: Add SDK integration tests

**Objective:** Test the user-facing functions (`Sequence()`, `Test()`, `Requires()`, `Expect()`, `Sync()`, `Once()`, `Exclusive()`) as they would be used in a real test script.

**Implementation guidance:**
- Test `Sequence(name, func)`:
  - Creates and registers a sequence
  - Duplicate name throws
- Test `Sequence(name)` (getter):
  - Returns a ScopeRequirement for that sequence
  - Must be called within a sequence
- Test `Test(name, func)`:
  - Creates and registers a test
  - Must be inside a sequence
- Test `Test(name)` (getter):
  - Returns a ScopeRequirement for that test
- Test `Requires(requirement)`:
  - With met requirement: no error
  - With unmet requirement: throws `UnmetRequirement`
- Test `Sync()`:
  - Returns a SyncRequirement
  - Registers it with the orchestrator
- Test `Expect(value, name)`:
  - Returns an Expectation object
  - Can chain methods
- Test `Once(func)`:
  - Function is called
  - Uses `__once` mock — verify Hash + __once are called
- Test `Exclusive(value, func)`:
  - Function is called via `__exclusive` mock
- Integration scenario:
  - Define a full test script (sequences with requirements, tests with expectations)
  - Run through generation → verify solution
  - Simulate execution → verify results

**Test requirements:** `ctest -R sdk` passes.

**Demo:** Full user-facing API verified, including a realistic multi-sequence test script scenario.

---

### Task 7: Add environment tests

**Objective:** Test the `Environment` setup API — UUT count, UUT values, teams, workers, script versioning, execution policy.

**Implementation guidance:**
- Create `tests/environment/CMakeLists.txt` and `tests/environment/test.cpp`
- Use the orchestrator fixture (needs Context.map, Context.team, Context.worker, etc.)
- Mock `__setExecutionPolicy` as a tracked call
- Mock `require("lua.core.can_open.object_dictionary")` if `AddIb` is tested (or skip IB-specific tests since they need CANopen)
- Test `Environment.Uut.Count(n)`:
  - Sets `Context.map.uuts` to `{1, 2, ..., n}`
- Test `Environment.UutValue.Add(key, default)`:
  - Registers a value with default in `Context.values`
  - `.Link(uut, value)` updates the value when UUT matches `Context.info.uut`
  - `.Link(uut, value)` is a no-op when UUT doesn't match (in execution stage)
  - Always updates in non-execution stage
- Test `Environment.Team.Add(...)`:
  - Sets `Context.team.hasTeam = true`
  - Registers players with correct leader and position
  - Same player in two teams throws
- Test `Environment.ScriptVersion()`:
  - Getter returns current version
  - Setter with string/number updates it
  - Invalid type throws
- Test `Environment.SetExecutionPolicy`:
  - `parallel` calls `__setExecutionPolicy(true)`
  - `sequential` calls `__setExecutionPolicy(false)`
  - Invalid value throws
- Test `Environment.Make(func)`:
  - Calls the function
  - Calls `team.Validate()` and `worker.Evaluate()`

**Test requirements:** `ctest -R environment` passes.

**Demo:** Full environment setup API verified.

---

### Task 8: Add team tests

**Objective:** Test the `Team` module — status, leadership, sync, validation.

**Implementation guidance:**
- Create `tests/team/CMakeLists.txt` and `tests/team/test.cpp`
- Set up Context.team with players and teams before each test
- Mock C++ team bindings: `Team.__tell`, `Team.__get`, `Team.__wait`, `Team.__done`, `Team.__sync`, `Team.__fail`
- Test `Team.HasTeam()`:
  - Returns false when no team configured
  - Returns true after `AddTeam`
- Test `Team.IsLeader()`:
  - Returns true for position 1 (first player in team)
  - Returns false for other positions
- Test `Team.GetLeader()`:
  - Returns the leader UUT number
- Test `Team.Position()`:
  - Returns the player's position in the team
- Test `Team.Sync(result)`:
  - When no team: does nothing
  - When team and `__sync` returns same status: result unchanged
  - When team and `__sync` returns `fail`: sets `result.pass = false` with "Teammate failure"
  - When team and `__sync` returns `critical_failure`: sets pass=false and throws TeamError
- Test `Team.Fail()`:
  - When team: calls `__fail`
  - When no team: no-op
- Test `Team.Wait(fun)`:
  - Only leader can call (throws for non-leader)
  - Must be a function (throws for non-function)
- Test `Team.Done()`:
  - Only teammate can call (throws for leader) — note: there may be a bug here, `Team.IsLeader` is compared without `()` in the source
- Test team validation (`team.Validate()`):
  - Player count matches UUT count: passes
  - Player count mismatch: throws

**Test requirements:** `ctest -R team` passes.

**Demo:** Team leadership, sync status propagation, and validation verified.

---

### Task 9: Wire together and verify

**Objective:** Build all new test modules, run the full suite, commit.

**Implementation guidance:**
- Add new subdirectories to `tests/CMakeLists.txt`
- Build all targets
- Run `ctest --test-dir build/vendor/frasy --output-on-failure` — all tests pass (previous 157 + new ones)
- Verify no regressions in existing tests

**Test requirements:** Full suite passes cleanly.

**Demo:** `ctest` shows all tests (old and new) passing.

---

## Important Implementation Notes

- The orchestrator module (`orchestrator.lua`) initializes `Context.orchestrator` on load. The fixture's `resetOrchestrator()` helper must clear this state between tests.
- The `scope_requirement/module.lua` dispatches based on `Context.info.stage` at require-time. Tests that need different stage behaviors should either set the stage before requiring, or load the specific stage file directly.
- `Team.__sync` and other C++ bindings are defined as Lua functions in team.lua itself (with empty bodies). The tests should override these with tracked mocks.
- `Team.Done()` has a likely bug: `assert(not Team.IsLeader, ...)` checks the function reference (always truthy) instead of `Team.IsLeader()`. Tests should document this.
- The `worker.Evaluate()` function populates `Context.worker.stages` — tests should verify the staging logic for various team/UUT configurations.
- `Environment.Make()` requires `Context.map`, `Context.team`, and calls `team.Validate()` + `worker.Evaluate()`, so the fixture must set up these structures.
