# Test Viewer

!!! warning "Not yet implemented"
    The Test Viewer panel is currently under development. The UI is present but functionality
    is incomplete.

**Hotkey:** ++f6++

---

## Intended Purpose

The Test Viewer will serve two functions:

### Live Execution Progress

During a test run, the Test Viewer will display the Solution tree — all sequences and tests —
with real-time status updates as they execute:

```
▶ Power On          [RUNNING]
  ✓ Supply Voltage  [PASSED]
  ▶ Current Draw    [RUNNING]
  ○ LED Check       [IDLE]
○ Functional        [IDLE]
  ○ Communication   [IDLE]
  ○ Data Transfer   [IDLE]
✓ Teardown          [DISABLED]
  ✓ Power Off       [DISABLED]
```

Each node will show its execution state (idle, running, passed, failed, skipped, disabled)
with color coding matching the UUT status icons.

### Enable/Disable Sequences and Tests

Before starting a run, operators will be able to toggle individual sequences and tests on or
off by clicking checkboxes in the tree. Disabled items are skipped during execution with the
reason "Disabled".

This is useful for:

- **Debugging** — run only the sequence you're working on.
- **Partial retesting** — skip sequences that already passed on a previous run.
- **Development** — disable tests that depend on hardware not yet connected.

---

## See Also

- [Test Lifecycle](../architecture/test-lifecycle.md) — how the Solution tree is structured
- [Sequences & Tests](../lua-reference/sequences-and-tests.md) — defining sequences and tests
- [Requirements](../lua-reference/requirements.md) — how disabled scopes interact with requirements
