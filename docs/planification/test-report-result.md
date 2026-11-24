# Implementation Plan — `result` field in test reports

## Problem Statement

Reports describe a scope's outcome with three booleans: `pass`, `skipped` and `enabled`.
They overlap and are often misread:

- A disabled scope is reported as `skipped = true, reason = "Disabled"` plus `enabled = false`.
- `pass` defaults to `false`, so skipped and disabled scopes count as failures. A disabled sequence fails the board.
- Tests inside a skipped or disabled sequence are never touched and keep the defaults (`pass = false`), so they read as failures.

## Requirements

- Every sequence and test gets a `result` string: `pass`, `fail`, `skipped` or `disabled`.
- Sequences and `info` aggregate their children by precedence: `fail` > `incomplete` > `pass`.
  - `disabled` child → `incomplete`.
  - `skipped` child → neutral (a skip means the scope did not apply to this run).
  - A test with a failed expectation that then hits an unmet requirement is `fail`, not `skipped`.
  - Team sync keeps sending anything but `pass` as a failure, so a skip on one UUT fails its teammates.
- Tests inside a skipped or disabled sequence inherit that state.
- Expectations get `result` = `pass` or `fail`.
- `info.format_version = 2`. Readers parse `format_version` first and pick the matching parser. A missing version means v1.
- Transition: v2 reports keep `pass`, `skipped` and `enabled`, computed from `result` and marked deprecated, so existing `onReport` hooks and external readers keep working. A later format version removes them.
- An `incomplete` board:
  - goes to a `debug` folder instead of `pass` or `fail`;
  - gets a dedicated UUT state, never shown as Passed;
  - skips the other output methods (`Report::Json`, `Report::Kvp`, `Report::Pdf`).
- Old (v1) reports still load in the result viewer and the analyzer.

## Public API impact (flagged)

| Change | Who is affected | Compatibility |
|---|---|---|
| `result` and `format_version` added to the report table given to `Context.map.onReport` | Every product | Additive; old fields kept for the transition |
| New `UutState::Incomplete` | Products that `switch` over `UutState` (e.g. the `demo_mode` app layer) | Non-exhaustive `switch` warning until handled |
| `debug` output folder | Anyone collecting the `pass`/`fail` folders | New folder only |
| `Report::*::makeReport` returns nothing for incomplete boards | Products calling them from `onReport` | Behaviour change for incomplete boards only |
| `result` added to headless, MCP and progress outputs | CI and MCP clients | Additive, `pass` kept |

`Orchestrator.HasPassed` and `HasBeenSkipped` keep their current semantics (a disabled scope counts as skipped).

## Task Breakdown

### Task 1: `result` on tests and sequences (Lua)

**Objective:** `ScopeResult` carries `result`; the orchestrator sets it and derives the deprecated fields from it.

**Implementation guidance:**
- `scope_result.lua`: add `result`. Derive `pass`, `skipped` and `enabled` from it in one place, at compile time.
- `orchestrator.lua` `RunTest` / `RunSequence`: set `result` on every path, including the error paths.
- When a sequence is skipped or disabled, set its tests to the same state.
- Aggregate sequence results by precedence.
- `HasPassed` / `HasBeenSkipped` and `Team.Sync` read `result`.
- Fix `test.result.reason = err` (stores the error table) so that `reason` is always a string.

**Tests:** `tests/orchestrator` — every path of each state, inheritance from skipped and disabled sequences, aggregation precedence, `HasPassed`/`HasBeenSkipped` unchanged.

**Docs:** `lua-reference/sequences-and-tests.md`, `architecture/test-lifecycle.md`.

---

### Task 2: `info.result` and `format_version` (Lua)

**Objective:** `CompileExecutionResults` writes `info.result` (`pass` / `fail` / `incomplete`), `info.format_version = 2` and the deprecated `info.pass`.

**Tests:** whole runs through the orchestrator fixture: all pass, one fail, one disabled test, one disabled sequence, a disabled test plus a failure (→ `fail`), a skipped-only sequence (→ neutral).

**Docs:** `developer-guide/creating-a-product.md` (`onReport`), `lua-reference/context.md`.

---

### Task 3: `result` on expectations (Lua)

**Objective:** Expectations get `result` = `pass` or `fail` next to their deprecated `pass`.

**Tests:** `tests/expectations*`: `result` matches `pass`, including inverted expectations.

**Docs:** `lua-reference/expectations.md`.

---

### Task 4: Shared report-result parsing (C++)

**Objective:** One place that turns a report node into a result, used by every C++ reader.

**Implementation guidance:**
- A `ScopeResult` enum (`Pass`, `Fail`, `Skipped`, `Disabled`, `Incomplete`) with string conversion.
- A function that reads `info.format_version` and returns the matching parser (v1 or v2).
  - v1 rebuilds the result from `pass` / `skipped` / `enabled`.
  - v2 reads `result`.
- Unknown versions and unknown strings raise a clear error.

**Tests:** new test module: v1 and v2 nodes for each state, a missing version, an unknown version, an unknown string.

---

### Task 5: Orchestrator outcome and folders (C++)

**Objective:** `checkResults` uses the shared parser and sends `incomplete` boards to `debug` with `UutState::Incomplete`.

**Implementation guidance:**
- Add `UutState::Incomplete` and handle it wherever `UutState` is mapped: main layer textures, MCP state strings, headless state.
- Handle it in the `demo_mode` app layer.

**Tests:** fixture-level test of the outcome → state and folder mapping.

**Docs:** `panels/test-viewer.md`, or wherever UUT states are described.

---

### Task 6: Report writers and formatters (C++)

**Objective:** `Report::Json`, `Report::Kvp` and `Report::Pdf` skip incomplete boards. The formatters print the result string (PASS / FAIL / SKIPPED / DISABLED / INCOMPLETE) instead of working it out from the booleans.

**Implementation guidance:** the JSON formatter's own output gains `result` and keeps `pass` / `skipped` / `enabled` for the transition.

**Tests:** formatter output for each state; `makeReport` writes nothing for an incomplete board.

**Docs:** `developer-guide/creating-a-product.md` (report outputs).

---

### Task 7: Result viewer and analyzer (C++)

**Objective:** Both load v1 and v2 reports through the shared parser.

**Implementation guidance:**
- The viewer shows the result string.
- The analyzer and `result_loader_saver` count per result value, adding Disabled and Incomplete.

**Tests:** load a v1 and a v2 report; analyzer counts for a mixed set.

**Docs:** `panels/result-viewer.md`, `panels/result-analyzer.md`.

---

### Task 8: Headless and MCP outputs (C++)

**Objective:** Headless runner, MCP runner, MCP HTTP layer and progress reporter emit `result` next to `pass`, and count passed tests from `result`.

**Tests:** `tests/headless`.

**Docs:** `getting-started/headless-mode.md`.

---

### Task 9: `demo_mode`

**Objective:** A demo product whose `onReport` reads `result` and `format_version`, with a sequence that exercises skipped and disabled scopes.

**Implementation guidance:** regenerate the hashes with `demo_mode/generate_hashes.bat`; build `demo_mode` with 0 new warnings.

## Out of scope

- Aborted or crashed runs. Tests that never ran because of a crash or `ErrorPolicy.stopAll` keep today's behaviour.
- Removing the deprecated fields (next format version).
