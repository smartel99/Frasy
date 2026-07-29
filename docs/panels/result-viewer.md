# Result Viewer

The Result Viewer displays pass/fail results from the **last completed test run**. It presents
a hierarchical view of sequences, tests, and individual expectations, automatically expanding
failed items so you can quickly identify what went wrong.

**Hotkey:** ++f4++

---

## Overview

After a test run completes, the Result Viewer loads the JSON report files from `logs/last/` and
presents them in a tree structure:

```
UUT 1 — PASSED
├── Power On — PASSED
│   ├── Supply Voltage — PASSED
│   │   └── Expectations (1)
│   │       └── VCC: pass, value=3.31, method=ToBeInPercentage, ...
│   └── Current Draw — PASSED
│       └── Expectations (1)
│           └── Idle Current: pass, value=0.042, method=ToBeLesser, ...
└── Functional — FAILED
    └── Communication — FAILED
        └── Expectations (1)
            └── ACK: FAIL, value=nil, method=ToBeEqual, expected=6
```

---

## Layout

### Tabs

When multiple UUTs are tested, each UUT gets its own tab. Tabs are labeled with the report
file name (typically the serial number or UUT identifier). Failed tabs are highlighted in
**red text**.

### Header

Each tab shows a summary header:

- **Serial Number** and **UUT number**
- **Overall result** (PASSED or FAILED)
- **Date** of the test run
- **Duration** in seconds
- **Script version**

### Tree Structure

Below the header, results are organized as a collapsible tree:

1. **Sequences** — top-level nodes
2. **Tests** — nested under their sequence
3. **Expectations** — nested under their test

---

## Auto-Expand on Failure

When the Result Viewer first loads results, it automatically expands:

- Failed sequences
- Failed tests within those sequences
- Failed expectations within those tests

Passed items remain collapsed. This lets you immediately see the failure chain without
clicking through the tree.

---

## Information Displayed

### Per Sequence

| Field | Description |
|---|---|
| Name | Sequence name |
| Result | PASSED / FAILED |
| Skipped | Whether the sequence was skipped (requirement unmet) |
| Enabled | Whether the sequence was enabled in the Test Viewer |
| Duration | Execution time in seconds |

### Per Test

| Field | Description |
|---|---|
| Name | Test name |
| Result | PASSED / FAILED |
| Skipped | Whether the test was skipped |
| Enabled | Whether the test was enabled |
| Duration | Execution time in seconds |

### Per Expectation

All fields from the expectation result are displayed as key-value pairs:

| Field | Description |
|---|---|
| `name` / `note` | The expectation label |
| `value` | The measured value |
| `pass` | Whether it passed |
| `inverted` | Whether `:Not()` was applied |
| `method` | The matcher used (e.g., `ToBeInPercentage`) |
| `expected`, `min`, `max`, `deviation`, `percentage`, `pattern` | Matcher-specific parameters |
| `extra` | Additional data from `:OnErrorExtra()` |

If the value is `nil` (e.g., a hardware read returned nothing), it is displayed explicitly.

---

## Auto-Open on Failure

The framework automatically opens the Result Viewer when a test run finishes with at least one
failed UUT. This behavior is implemented in `MainApplicationLayer` and can be customized in
your subclass.

---

## Live Reload

The Result Viewer monitors the `logs/last/` directory for file changes. If report files are
modified (e.g., by a subsequent test run finishing), the viewer reloads automatically without
needing to close and reopen the panel.

---

## Report Source

Results are loaded from JSON files in:

```
logs/last/
  <report_1>.json
  <report_2>.json
  ...
```

These files are overwritten on every test run. Historical results are preserved in
`logs/pass/` and `logs/fail/` directories (organized by outcome). The Result Viewer only
shows the `logs/last/` files.

---

## Tips

- **Check failed expectations first.** The auto-expand feature directs you straight to the
  problem — look at the `value` field to see what was measured versus what was expected.
- **Use `:OnErrorExtra()` for context.** If failures are hard to diagnose from the value alone,
  attach extra debugging info (raw bytes, retry counts, timestamps) to your expectations.
- **Compare with the Log Window.** If an expectation shows `value: nil`, the Log Window
  (++f2++) often reveals the underlying hardware error (timeout, communication failure).
- **Use the Result Analyzer (++f5++) for trends.** If a value passes but is drifting toward a
  limit, the Result Analyzer will show it statistically across multiple runs.

---

## See Also

- [Result Analyzer](result-analyzer.md) — statistical analysis across multiple runs
- [Expectations](../lua-reference/expectations.md) — how expectations produce results
- [Test Lifecycle](../architecture/test-lifecycle.md) — report generation and storage
