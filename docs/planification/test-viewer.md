# Test Viewer — Implementation Plan

Live, per-UUT view of the solution tree (sequences → tests) with coloured status bullets.
Builds on the existing skeleton in `Frasy/src/layers/test_viewer.{h,cpp}` (F6).

## Requirements

- Full tree is visible **before** a run starts: generated as soon as an environment is loaded.
- Bullets (same for sequences and tests):

  | State | Bullet |
  |---|---|
  | Not run yet | hollow ring, text colour |
  | Disabled | filled gray |
  | Skipped (unmet requirement) | hollow gray ring |
  | Running | filled orange |
  | Passed | filled green |
  | Failed / error | filled red |

- Sequences collapsed by default, auto-expand when they start, auto-collapse when they **pass**
  (failed ones stay open). Manual toggling by the operator is otherwise respected.
- UUT selector with `<` `>` arrows + combo, same pattern as `ResultAnalyzer`.
- Enable/disable checkboxes actually gate execution (locked while running).

## Current state (findings)

| Area | Status |
|---|---|
| `Models::Solution` | `unordered_map`s → **execution order is lost**. Generation inserts `Sequence{}` / `Test{}` → `enabled = false` by default. |
| `Sequence::state` / `Test::state` | `std::vector<ExecutionState>` already exists (per-UUT), never filled. |
| `MainApplicationLayer` | Doesn't override `GetSolution()` (viewer always sees the empty static one); `generate()` is commented out. |
| Environment loading | Owned by the derived app (`makeOrchestrator()` in demo, `loadProduct()` for MCP), both via `ProductProvider::setup()` → `Orchestrator::loadUserFiles()`. `MainApplicationLayer` gets no notification. |
| `Orchestrator::generate()` | Shares `m_running` with `runSolution()`: a run started during generation is rejected with a modal "already running" dialog, and `generate()` pops a modal "busy" dialog. |
| `__progress` | Added for headless mode (0129f9b). Only bound in the execution stage when `m_progressCallback` is set, which only `HeadlessRunner` does (feeds `ProgressReporter` → text/JSON on stdout). No-op in GUI and MCP. |
| Disabled/skipped | Disabled test emits `test_end(pass=true)` → indistinguishable from pass. Disabled sequence emits `sequence_start` and never `sequence_end`. |
| Aborts | A test/sequence that `error()`s out (stopAll, Lua error) never emits its `_end` → would stay orange forever. |
| `enable_list` | Gating works in Lua (`IsScopeEnabled`) but is reset to `{}` in `load_tests.lua` and never populated. `Enable/Disable` write `s[test] = bool` while `IsScopeEnabled` reads `test.enabled` → runtime error; missing test entry also errors. |
| Threading | `m_solution` is written by the generation thread and would be written by UUT threads; UI reads it unlocked. |

## Phases

Ordered so each phase builds and runs on its own, and the first visible result (tree before a run) lands early.
One commit per phase on `feature/test-viewer`.

```
1 Model ──► 2 Generate on load ──► 3 State tracking ──► 4 Lua events ──► 5 Viewer UI ──► 7 Docs
   └──────────────────────────────► 6 Enable wiring (independent of 3–5)
```

### Phase 1 — Model

Foundation, no visible change. Everything else reads these structures.

- `ExecutionState`: add `skipped` (unmet requirement), keep `disabled` for operator/Lua disable.
- `Solution`: add `std::vector<std::string> sequenceOrder`; `Sequence`: add `std::vector<std::string> testOrder`.
  Filled in `runStageGenerate()` by walking sections → stages → subsequences → test stages, first-appearance order,
  deduped (a sequence can be split across sections).
- Generation creates entries as enabled, and **preserves existing enable flags** on regenerate.
- `Solution::ResetStates(std::size_t uutCount)`: resize `state` vectors to `uutCount + 1` (UUTs are 1-based),
  set each to `disabled` if not enabled, else `idle`.

**Done when:** generating the demo product gives the expected order, all entries enabled.

### Phase 2 — Generate on environment load

First visible milestone: the existing skeleton viewer shows the tree as soon as a product is picked.
Needs Phase 1, or every entry shows as disabled.

- Orchestrator:
  - `std::atomic<std::uint64_t> m_loadId`, bumped at the end of a successful `loadUserFiles()`; `getLoadId()`.
  - Split generation from runs: own `std::future<void> m_generating` and `std::mutex m_generateMutex`
    held for the whole of `runStageGenerate()`, so background and run-time generation serialize.
  - `runSolution()` no longer collides with a background generation: its worker waits on `m_generating`,
    then `runStageGenerate(regenerate)` returns immediately when already generated.
  - `loadUserFiles()` waits for an in-flight generation before resetting state (fast product switching).
  - `requestGenerate()`: silent no-op if running or already generating (no modal dialog).
  - `std::mutex m_solutionMutex` + `std::atomic<std::uint64_t> m_solutionRevision`;
    `getSolution()` returns a copy under lock. Needed now because the UI reads while the generation thread writes.
- `MainApplicationLayer`:
  - `onUpdate()`: when `getLoadId()` differs from the last seen one, call `requestGenerate()`.
    Works for every app without the derived class having to call anything, and stays on the main thread.
  - Override `GetSolution()`; `generate()` → `requestGenerate()`.

**Done when:** picking a product (dropdown, startup, or MCP `load_product`) fills the viewer; MCP
`load_product` immediately followed by `run_tests` runs normally; headless output unchanged.

### Phase 3 — Orchestrator state tracking

C++ side of live progress. Needs Phase 1 (state vectors) and Phase 2 (mutex/revision).

- Always bind `__progress.report` in the execution stage. The binding updates `m_solution` states under lock,
  bumps the revision, then forwards to `m_progressCallback` if set (headless unchanged).
- Handle new types `sequence_disabled`, `sequence_skipped`, `test_disabled`, `test_skipped`.
  `test_end` / `sequence_end` never overwrite a `disabled`/`skipped` state.
- `runSolution()` calls `ResetStates()` at start.
- End of `runStageExecute()`: any node still `running` for a UUT → `error`; anything still `idle` stays
  empty (never reached, e.g. after a stopAll).
- UUT marked `Disabled` → all its nodes `disabled`.

**Done when:** a run in the demo leaves correct final states (checked in the debugger or a temporary dump).

### Phase 4 — Lua events (`orchestrator.lua`)

Small Lua-only change. Harmless alone; meaningful once Phase 3 consumes it.

- `RunSequence`: report `sequence_disabled` / `sequence_skipped` in the skip branches.
- `RunTest`: report `test_disabled` / `test_skipped` before the existing `test_end`.
- Emit `test_end(false)` before each `error(...)` re-raise, so aborts don't rely only on the C++ finaliser.
- Headless drops unknown types (`else { return; }`), so its stdout stays the same.

### Phase 5 — Viewer UI (`test_viewer.{h,cpp}`)

Needs 1–4 for real data, but can be developed against 1–3.

- Interface: `getSolution()` by value + `getSolutionRevision()`, `getUutCount()`, `isRunning()`,
  `getUutState(uut)` (to tag disabled UUTs in the combo).
- Header: `<` combo `>` UUT selector; combo entries show a bullet with the UUT's overall state.
- Body: 2-column table (bullet+tree, enable checkbox). Sequences via `TreeNodeEx`, tests as
  `Leaf | NoTreePushOnOpen`, iterated via `sequenceOrder` / `testOrder`.
- Bullet: `ImDrawList::AddCircle` (hollow) / `AddCircleFilled`, colours per the table above;
  tooltip with the state name.
- Auto fold: per-UUT cache of the last seen sequence state; on `idle→running` → `SetNextItemOpen(true)`,
  on `running→passed` → `SetNextItemOpen(false)`. Only on transitions, so manual toggles stick.
  Switching UUT re-applies the rule from scratch for that UUT (running/failed open, others closed).
- Optional: `SetScrollHereY()` when a test becomes running on the selected UUT.
- Checkboxes inside `BeginDisabled(isRunning())`.

### Phase 6 — Enable/disable wiring

Independent of 3–5; only needs Phase 1 (enabled defaults + preservation on regenerate).
Could be split into its own PR if preferred.

- After `loadTests()` in both validation and execution states, push `m_solution` flags into
  `Context.orchestrator.enable_list` as `{ [seq] = { enabled = b, [test] = { enabled = b } } }`.
- Fix `Orchestrator.Enable/Disable` to write that same shape; `IsScopeEnabled` defaults a missing test entry to `true`.

### Phase 7 — Docs

- Rewrite `docs/panels/test-viewer.md` (drop the "not yet implemented" warning, document bullets/selector/folding).

## Verification

Manual in `demo_mode`: multi-UUT run, one test disabled, one sequence disabled, one forced failure, one stopAll
abort, one unmet requirement; quick product switching; MCP `load_product` + `run_tests` back-to-back.
Check headless text and JSON output is unchanged.
