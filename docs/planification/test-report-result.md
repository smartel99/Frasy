# Implementation Plan — `result` field in test reports

## Problem Statement

Reports describe a scope's outcome with three booleans: `pass`, `skipped` and `enabled`.
They overlap and are often misread:

- A disabled scope is reported as `skipped = true, reason = "Disabled"` plus `enabled = false`.
- `pass` defaults to `false`, so skipped and disabled scopes count as failures. A disabled sequence fails the board.
- Tests inside a skipped or disabled sequence are never touched and keep the defaults (`pass = false`), so they read as failures.

## Requirements

- Every sequence and test gets a `result` string: `pass`, `fail`, `skipped` or `disabled`.
- Sequences aggregate their tests by precedence: `fail` > `incomplete` > `pass`.
  - `disabled` child → `incomplete`.
  - `skipped` child → neutral (a skip means the scope did not apply to this run).
  - A test with a failed expectation that then hits an unmet requirement is `fail`, not `skipped`.
  - Team sync keeps sending anything but `pass` as a failure, so a skip on one UUT fails its teammates.
- `info` aggregates by precedence: `incomplete` > `fail` > `pass`: any disabled sequence or test makes the board a
  debug run, even when something failed; the sequences keep showing which ones failed.
- Tests inside a skipped or disabled sequence inherit that state.
- Expectations get `result` = `pass` or `fail`.
- `info.format_version = 2`. Readers parse `format_version` first and pick the matching parser. A missing version means v1.
- Report scheme: a product picks its report layout with `SetReportFormat(ReportFormat.v1 | ReportFormat.v2)` in
  `Environment.Make`. The default is v1, so existing products keep working until they opt in.
  - v1: `pass`, `skipped` and `enabled`, no `result` nor `format_version`. An incomplete board has `info.pass = false`.
  - v2: `format_version = 2` and `result` only, expectations included.
  - The scheme only changes what is written: the orchestrator keeps `result` internally, incomplete boards still go
    to `debug/`, skip the other writers and get the incomplete UUT states, and the Lua expectation helpers are
    unchanged.
  - Headless, MCP and progress outputs don't follow the scheme: they carry `result` and `pass`.
- An `incomplete` board:
  - goes to a `debug` folder instead of `pass` or `fail`;
  - gets a dedicated UUT state, never shown as Passed: `IncompletePassed` if everything that ran passed,
    `IncompleteFailed` if a sequence or test failed;
  - skips the other output methods (`Report::Json`, `Report::Kvp`, `Report::Pdf`). Checking `info.result` is enough,
    since it is `incomplete` whenever anything was disabled.
- Headless exit codes: 0 pass, 1 fail (incomplete runs where something failed included), 2 error, 3 incomplete.
- Old (v1) reports still load in the result viewer and the analyzer.

## Public API impact (flagged)

| Change | Who is affected | Compatibility |
|---|---|---|
| `SetReportFormat(ReportFormat.v2)` gives the report table and files `result` and `format_version`, without `pass`, `skipped` and `enabled` | Products opting in | v1 is the default: no change until a product opts in |
| New `UutState::IncompletePassed` and `IncompleteFailed`, plus `RunVerdict` helpers | Products that `switch` over `UutState` (e.g. the `demo_mode` app layer) | Non-exhaustive `switch` warning until handled |
| A board with a failure and something disabled is `IncompleteFailed` in `debug/`, no longer `Failed` in `fail/` | Products testing `== UutState::Failed` | Use `getVerdict()` |
| Headless exit code `3` and `INCOMPLETE_PASS` / `INCOMPLETE_FAIL` states | CI scripts | New values only |
| `debug` output folder | Anyone collecting the `pass`/`fail` folders | New folder only |
| `Report::*::makeReport` returns nothing for incomplete boards, and removes the stale `last_uut<N>` file | Products calling them from `onReport` | Behaviour change for incomplete boards only |
| Report writers read `result` rather than `pass` | `onReport` hooks that change `pass` before calling a writer | Hooks must set `result` too |
| `result` added to headless, MCP and progress outputs | CI and MCP clients | Additive, `pass` kept |
| Progress `test_end` has `pass: false` for skipped and disabled tests (was `true`) | CI scripts reading `pass` from progress events | Read `result` to tell skipped and disabled from failed |
| `Orchestrator::setProgressCallback` gets a `Report::Result` instead of `bool pass`; disabled and skipped tests no longer report as passed | Products installing their own progress callback | Compile error until the callback takes `Report::Result` |
| Analyses saved by the result analyzer store per-result counts | Older Frasy versions loading them | Older versions can't load them; older files still load |

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
- `Expectation::fromTable` (`Frasy/src/utils/lua/expectation.cpp`) inverts `pass` a second time when `inverted` is
  set, although Lua already applied it; live expectation views show inverted expectations backwards. Read `result`
  there and drop the extra inversion.

**Tests:** new test module: v1 and v2 nodes for each state, a missing version, an unknown version, an unknown string.

---

### Task 5: Orchestrator outcome and folders (C++)

**Objective:** `checkResults` uses the shared parser and sends `incomplete` boards to `debug` as `IncompletePassed` or
`IncompleteFailed`, depending on whether a sequence or test failed.

**Implementation guidance:**
- Add both states and handle them wherever `UutState` is mapped: main layer textures (one icon each), MCP state strings,
  headless state, labels and exit code. Share the severity rule (`RunVerdict`) instead of repeating it.
- Handle them in the `demo_mode` app layer.

**Tests:** fixture-level test of the outcome → state and folder mapping.

**Docs:** `panels/test-viewer.md`, or wherever UUT states are described.

---

### Task 6: Report writers and formatters (C++)

**Objective:** `Report::Json`, `Report::Kvp` and `Report::PDF` skip incomplete boards (`info.result == incomplete`, which covers failed boards with anything disabled). The formatters print the result string (PASS / FAIL / SKIPPED / DISABLED / INCOMPLETE) instead of working it out from the booleans.

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

### Task 9: Report scheme selection

**Objective:** `SetReportFormat` in `Environment.Make` picks the layout of the report given to `onReport` and saved.

**Implementation guidance:**
- `ReportFormat = { v1 = 1, v2 = 2 }`, stored in `Context`; default v1. An unknown value is an error at `Make` time.
- `CompileExecutionResults` and `AddExpectationResult` write the chosen layout from `result`, before `onReport`.
- The JSON formatter writes the same layout; the writers and formatters read either one (`getResult(sol::table)`).

**Tests:** both layouts of `info`, sequences, tests and expectations; default is v1; an unknown format is an error;
`onReport` and the writers see the chosen layout; a v1 incomplete board has `info.pass = false` and still goes to
`debug/`.

**Docs:** `creating-a-product.md` (the setter and both layouts), `test-lifecycle.md` report example.

---

### Task 10: `demo_mode`

**Objective:** A demo product on `ReportFormat.v2` whose `onReport` reads `result` and `format_version`, with a sequence that exercises skipped and disabled scopes.

**Implementation guidance:** regenerate the hashes with `demo_mode/generate_hashes.bat`; build `demo_mode` with 0 new warnings.

## Out of scope

- Aborted or crashed runs. Tests that never ran because of a crash or `ErrorPolicy.stopAll` keep today's behaviour.
