# Expect `opt` Table Extension — mandatory & inverted fields

**Date:** 2026-08-03  
**Related issue:** [#13 — Expectation:Mandatory() should enforce retroactively](https://github.com/smartel99/Frasy/issues/13)  
**Status:** Planned

---

## Problem Statement

Currently, `Expect(value, name, opt?)` only supports `note` and `extra` in the `opt` table.
Modifiers like `:Mandatory()` and `:Not()` are chain-only methods that depend on call order
(issue #13). We want to allow `mandatory` and `inverted` to be declared in the `opt` table,
making the API more declarative and order-independent. The chain methods will be deprecated with
a `Log.W()` warning, while the `opt` table becomes the recommended approach. Unknown fields in
`opt` will trigger a strict validation error.

Additionally, chaining multiple `ToBe*` matchers on a single Expectation is currently silently
allowed (each one adds a separate result to the orchestrator), which is confusing and almost
certainly a bug. This will be forbidden with a clear error.

---

## Requirements

1. `Expect(value, "x", { mandatory = true })` sets mandatory behavior at construction time
2. `Expect(value, "x", { inverted = true })` sets inverted (Not) behavior at construction time
3. Unknown fields in `opt` produce a hard error (strict validation)
4. `:Mandatory()` and `:Not()` chain methods continue to work but emit `Log.W()` deprecation
   warnings
5. All three stage modules (generation, validation, execution) handle the new fields
6. Documentation is updated to reflect the preferred `opt` table approach
7. The issue #13 ordering bug is noted/referenced but not fixed as part of this work
8. Chaining multiple `ToBe*` matchers on a single Expectation is forbidden with a clear error

---

## Background

- `ExpectationResult:New(value, name, opt)` currently extracts `opt.note` and `opt.extra`
- `Expectation:New(value, name, opt)` in `execution.lua` sets `mandatory = false` and creates
  the result
- The `generation.lua` and `validation.lua` modules are no-op stubs that return `self` for all
  methods
- The `ExpectationResultOpt` type annotation only declares `note` and `extra`
- Tests use Google Test with sol2 bindings, following the `OrchestratorTestFixture` pattern
- `Is` and `Maybe` utilities exist for field validation; `CheckField` provides strict checking
- Every `ToBe*` method currently returns `self`, allowing erroneous double-matcher chains

---

## Proposed Solution

1. Add an `opt` validation function that checks all fields against an allowed set (`note`,
   `extra`, `mandatory`, `inverted`) and errors on unknowns
2. Extend `ExpectationResult:New` to read `opt.mandatory` and `opt.inverted` and pass them
   through
3. Extend `Expectation:New` (execution) to initialize `mandatory` and `result.inverted` from
   the opt values
4. Add a guard preventing multiple `ToBe*` calls on a single Expectation
5. Add deprecation warnings to `:Mandatory()` and `:Not()` in the execution module
6. Update generation/validation modules for API consistency
7. Update documentation and type annotations
8. Write integration tests for the full workflow

---

## Task Breakdown

### Task 1: Add `opt` table validation utility function

**Objective:** Create a reusable validation function that checks an `opt` table against a set
of allowed keys and errors on unknown fields.

**Implementation:** Add a local function `validateOpt(opt, allowed)` in a shared location
(e.g., a new file `expectation/validate_opt.lua` or inline in each module) that iterates `opt`
keys and calls `error()` for any key not in the allowed set. The allowed keys for Expect are:
`note`, `extra`, `mandatory`, `inverted`.

**Test requirements:**

- Valid opt tables with known fields pass without error
- An opt table with a typo like `{ mandatroy = true }` errors with a clear message
- An opt table with `nil` still works (no validation needed)
- An opt table with only `note` or `extra` still works (backward compatibility)

---

### Task 2: Extend `ExpectationResult:New` to support `mandatory` and `inverted` in `opt`

**Objective:** Make `ExpectationResult:New` read `opt.mandatory` (boolean) and `opt.inverted`
(boolean) and store them in the result object.

**Implementation:** In `result.lua`, update the `ExpectationResultOpt` type annotation to
include `mandatory` and `inverted`. In `ExpectationResult:New`, when `opt` is a table, check
for `opt.inverted` (boolean) and apply it to the `inverted` field. Return `mandatory` as a
separate value or store it for the caller to use.

**Test requirements:**

- `ExpectationResult:New(1, "x", { inverted = true })` creates a result with `inverted = true`
- `ExpectationResult:New(1, "x", { mandatory = true })` — the mandatory info is accessible
- Default behavior unchanged when opt is nil or has neither field

---

### Task 3: Extend `Expectation:New` (execution) to apply `opt.mandatory` and `opt.inverted`

**Objective:** Wire up the new opt fields so they take effect in the execution module's
`Expectation:New`.

**Implementation:** In `execution.lua`, modify `Expectation:New` to check if `opt` contains
`mandatory = true` and set `self.mandatory = true`. For `inverted`, it's already handled by the
result (from Task 2). Call `validateOpt` (from Task 1) within `Expectation:New`.

**Test requirements:**

- `Expect(false, "x", { mandatory = true }):ToBeTrue()` throws UnmetExpectation
- `Expect(1, "x", { inverted = true }):ToBeEqual(2)` — raw pass is false, inverted is true
  (effective pass)
- `Expect(1, "x", { mandatory = true, inverted = true }):ToBeEqual(1)` — pass=true,
  inverted=true → effective fail → throws
- Backward compatibility: `Expect(true, "x"):ToBeTrue()` still works

---

### Task 4: Forbid chaining multiple matcher (`ToBe*`) calls on a single Expectation

**Objective:** Error if a user calls a second `ToBe*` method on the same Expectation instance.
Only one matcher per `Expect()` is valid.

**Implementation:** Add a field (e.g., `self.asserted = false`) to the Expectation instance at
construction. At the start of every `ToBe*` method in `execution.lua`, check
`if self.asserted then error(...)`. Set `self.asserted = true` after the assertion runs. Apply
the same guard in `generation.lua` and `validation.lua` for consistency (error early regardless
of stage).

**Test requirements:**

- `Expect(1, "x"):ToBeEqual(1):ToBeTrue()` errors with a clear message like
  "Expectation already asserted"
- `Expect(1, "x"):ToBeEqual(1)` alone still works
- `Expect(1, "x"):ToBeEqual(1):ExportAs("y")` still works (ExportAs is post-assertion, not a
  matcher)
- `Expect(1, "x"):ToBeEqual(1):OnErrorExtra({})` still works
- Applies across all stages (generation, validation, execution)

---

### Task 5: Add deprecation warnings to `:Mandatory()` and `:Not()` chain methods

**Objective:** Emit `Log.W()` deprecation messages when users call the chain methods, guiding
them toward the `opt` table.

**Implementation:** In `execution.lua`, add
`Log.W("Expectation:Mandatory() is deprecated. Use Expect(value, name, { mandatory = true }) instead.")`
at the start of the `Mandatory()` method. Similarly for `Not()`. The methods still work as
before.

**Test requirements:**

- Calling `:Mandatory()` logs a warning (verify via mock `Log.W` capture)
- Calling `:Not()` logs a warning
- Chain methods still function correctly (existing tests pass)

---

### Task 6: Update generation and validation modules for consistency

**Objective:** Ensure `generation.lua` and `validation.lua` accept and handle the new opt
fields without breaking (they are no-op modules but should validate opt and accept the fields).

**Implementation:** Add opt validation in their `New` methods. They don't need to act on
`mandatory`/`inverted` since they return self for everything, but they should validate opt to
catch typos during generation stage. Also add deprecation warnings to their `:Mandatory()` and
`:Not()` stubs.

**Test requirements:**

- `Expect(1, "x", { mandatory = true })` during generation stage doesn't error
- `Expect(1, "x", { typo = true })` during generation stage errors
- Deprecation warnings fire in generation/validation stages too

---

### Task 7: Update documentation and type annotations

**Objective:** Update the expectations documentation and Lua type annotations to reflect the
new `opt` fields as the recommended approach.

**Implementation:**

- Update `docs/lua-reference/expectations.md` to show opt table as primary API, chain methods
  as deprecated
- Update `ExpectationResultOpt` class annotation in `result.lua`
- Add `@deprecated` annotations to `:Mandatory()` and `:Not()` method docs
- Note issue #13 (ordering bug) in a "Known Issues" or "Migration" section — the opt table
  approach sidesteps it

---

### Task 8: Integration test — full chain with opt table in SDK context

**Objective:** Add an integration test that exercises the full workflow using the `Expect()` SDK
function with the new opt table fields.

**Implementation:** Add test cases in `tests/sdk/expect.cpp` that use
`Expect(value, name, { mandatory = true })` and `Expect(value, name, { inverted = true })`
within the full SDK context.

**Test requirements:**

- `Expect(true, "x", { mandatory = true }):ToBeTrue()` passes cleanly
- `Expect(false, "x", { mandatory = true }):ToBeTrue()` throws
- `Expect(1, "x", { inverted = true }):ToBeEqual(2)` stores inverted=true in result
- `Expect(1, "x", { unknown_field = 1 }):ToBeEqual(1)` errors with validation message
- `Expect(1, "x"):ToBeEqual(1):ToBeTrue()` errors (double matcher guard)

---

## API Examples (After Implementation)

### New recommended style

```lua
-- Mandatory expectation via opt table
Expect(connected, "Board Connected", { mandatory = true }):ToBeTrue()

-- Inverted expectation via opt table
Expect(errorBit, "Error Bit", { inverted = true }):ToBeTrue()

-- Both mandatory and inverted
Expect(status, "No Error", { mandatory = true, inverted = true }):ToBeEqual(0xFF)

-- With note and extra (existing fields still work)
Expect(voltage, "VCC", {
    mandatory = true,
    note = "Main supply rail",
    extra = { channel = 3 }
}):ToBeInPercentage(3.3, 5.0)
```

### Deprecated style (still works, emits Log.W)

```lua
-- These still function but produce deprecation warnings:
Expect(connected, "Board Connected"):Mandatory():ToBeTrue()
Expect(errorBit, "Error Bit"):Not():ToBeTrue()
```

### Forbidden (new error)

```lua
-- Double matcher — now an error:
Expect(1, "x"):ToBeEqual(1):ToBeTrue()  -- ERROR: "Expectation already asserted"
```

---

## Note on Issue #13

Issue #13 describes a bug where `:Mandatory()` called *after* a matcher has no effect because
`enforce()` is only called inside the matcher methods. The `opt` table approach sidesteps this
entirely — when `mandatory` is set in the opt table, it's configured before any matcher runs,
so enforcement always works correctly.

The issue #13 bug (making `:Mandatory()` enforce retroactively) is **not** fixed as part of
this work. The deprecated `:Mandatory()` chain method retains its current behavior. Users who
hit this ordering issue should migrate to the opt table approach.
