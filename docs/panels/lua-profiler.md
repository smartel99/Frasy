# Lua Profiler

The Lua Profiler provides per-function timing data for Lua test scripts. It tracks every
function call during test execution and displays hit counts, total time, average time, and
time distribution — helping you identify performance bottlenecks in your test sequences.

**Hotkey:** ++f8++

---

## Overview

The profiler instruments all Lua function calls automatically using Lua's debug hooks. Every
function entry and exit is recorded with microsecond-precision timestamps. The results are
displayed in a hierarchical table that mirrors the call tree.

---

## Display

### Thread Tabs

Each UUT runs in its own thread, and each thread gets its own profiler tab. Tabs are labeled
with the thread name (e.g., `UUT1`, `UUT2`). Hover over a tab to see:

- Thread ID
- Total profiled time
- Top function (most time spent)
- Source location

### Table Columns

| Column | Description |
|---|---|
| **Name** | Function name (collapsible — click to expand child calls) |
| **Hit Count** | Number of times the function was called |
| **Total Time** | Cumulative time in this function (% of total + microseconds) |
| **Average Time** | Mean duration per call (% of total + microseconds) |
| **Source** | File and line number (hidden by default) |
| **Min Time** | Shortest observed call duration |
| **Max Time** | Longest observed call duration |
| **Graph** | Button to open a scatter plot of call durations over time |

### Call Tree

The table is hierarchical — functions called from within another function appear as indented
children. Expand a row to see what it calls internally:

```
MeasureVoltage          50 hits   120ms total   2.4ms avg
  ├── Ib:Upload         50 hits   100ms total   2.0ms avg
  │   └── __upload      50 hits    95ms total   1.9ms avg
  └── CheckField        50 hits     5ms total   0.1ms avg
```

---

## Graphs

Click the **Graph** button on any row to open a scatter plot window showing individual call
durations over time (or by occurrence index).

The graph displays:

- **X axis** — occurrence index (sample mode) or wall-clock timestamp
- **Y axis** — call duration in microseconds
- **Horizontal line** — average time

### Display Modes

Toggle **"Display as samples"** to switch between:

- **Samples mode** — X axis is the call index (1, 2, 3, ...). Good for seeing patterns.
- **Timestamp mode** — X axis is wall-clock time. Good for correlating with other events.

This is useful for spotting:

- Occasional slow calls (spikes above the average)
- Warming/cooling trends in hardware communication
- Timeout retries (sudden jumps in duration)

---

## Controls

| Button | Action |
|---|---|
| **Reset All** | Clears all profiling data across all threads |
| **Reset** (per thread) | Clears data for a single thread |
| **Dump Trace** | Exports the profiling data as a JSON trace file for external analysis |

### Dump Trace

The "Dump Trace" button saves the profiling data to a JSON file. This can be loaded into
trace visualization tools for detailed offline analysis.

---

## What Gets Profiled

The profiler captures:

- All Lua function calls (both your test scripts and the framework SDK)
- Framework-level operations (sequence start/end, test start/end)
- Custom Lua functions exposed from C++

Profiling is active during all three stages (generation, validation, execution), but the most
useful data comes from the execution stage where real hardware I/O occurs.

---

## Performance Impact

The profiler adds overhead to every function call due to the Lua debug hook. During normal test
execution this is negligible (microseconds per call), but for extremely tight loops with
millions of iterations it could be noticeable.

The profiler is always enabled when the application is running. Use "Reset All" before a
profiling run to get clean data.

---

## Common Use Cases

### Finding Slow Tests

1. Run your test sequence.
2. Open the Lua Profiler (++f8++).
3. Sort by **Total Time** (click the column header).
4. The top entries are where most time is spent — focus optimization efforts there.

### Identifying Hardware Bottlenecks

Look for SDO operations (`Ib:Upload`, `Ib:Download`, `__upload`, `__download`) with high
average times. If a single SDO read takes 50ms but you're doing 200 of them, that's 10 seconds
of bus time.

### Detecting Timeout Retries

Open the graph for an SDO operation. If you see occasional spikes at the timeout value (e.g.,
1000ms), it means the operation is timing out and retrying. Investigate the hardware
communication.

### Comparing Before/After Optimization

1. Reset the profiler.
2. Run the sequence.
3. Note the total time and function averages.
4. Make your optimization.
5. Reset and re-run.
6. Compare the numbers.

---

## Tips

- **Reset before profiling.** Old accumulated data from startup and previous runs will skew
  your percentages. Always reset before the run you want to analyze.
- **Look at hit count × average time.** A function with a 1ms average but 500 hits contributes
  500ms. Reducing hit count (caching, batching) can be more effective than optimizing the
  function itself.
- **Expand the call tree.** A slow function might just be a wrapper — expand it to find which
  child call is actually slow.
- **Use graphs for intermittent issues.** If a test passes but occasionally takes too long,
  the scatter plot will show the outlier calls clearly.
- **The Source column helps locate code.** Enable it (right-click header → show Source) to
  see exactly which file and line each function is defined at.

---

## See Also

- [Test Lifecycle](../architecture/test-lifecycle.md) — how sequences and tests are executed
- [Hardware Communication](../architecture/hardware.md) — SDO operations that often dominate profiling data
- [Logging](../lua-reference/logging.md) — use `Log.D` alongside profiling to correlate timing with events
