# Result Analyzer

The Result Analyzer performs **statistical analysis** across multiple test runs. It aggregates
expectation values from all reports in `logs/pass/` and `logs/fail/`, computes statistics
(mean, standard deviation, Pp, Ppk), and renders histograms — helping you identify measurement
drift, process capability issues, and intermittent failures.

**Hotkey:** ++f5++

---

## Overview

While the [Result Viewer](result-viewer.md) shows a single run's pass/fail results, the Result
Analyzer looks at **trends over time**. It answers questions like:

- Is a measurement drifting toward its limit?
- What is the process capability (Pp/Ppk) for a given expectation?
- Which tests have the lowest yield?
- How spread are the measured values?

---

## Workflow

1. **Open** the Result Analyzer (++f5++).
2. **Configure filters** (optional) — narrow down to specific serials, UUTs, sequences, or tests.
3. **Click "Generate"** — the analyzer scans all report files and computes statistics.
4. **Review results** — browse the hierarchical view with statistics and histograms.

---

## Filter Options

Before generating, you can narrow the analysis scope:

| Filter | Description |
|---|---|
| **Serial Numbers** | Only analyze reports with these serials. Empty = all. |
| **Locations (UUTs)** | Only analyze specific UUT positions. Empty = all. |
| **Sequences** | Only include these sequences. Empty = all. |
| **Tests** | Only include these tests. Empty = all. |

### Combine All Locations

When **checked** (default): all UUT positions are combined into a single statistical pool.

When **unchecked**: statistics are computed per-UUT position separately. Useful when different
UUT slots have different measurement characteristics (e.g., different routing, different
tolerances).

---

## Statistics Computed

### Per Test / Per Sequence

| Statistic | Description |
|---|---|
| Total | Number of times this scope was executed |
| Passed | Number of times it passed |
| Pass % | Pass rate as a percentage |
| Enabled | How often it was enabled |
| Skipped | How often it was skipped (requirement unmet) |
| Average Duration | Mean execution time |

### Per Expectation (Numeric Values)

For expectations with numeric matchers (`ToBeInRange`, `ToBeInPercentage`, `ToBeNear`,
`ToBeGreater`, `ToBeLesser`, etc.):

| Statistic | Description |
|---|---|
| Total | Number of samples |
| Passed | Number that passed the expectation |
| Pass % | Yield for this expectation |
| Min Observed | Lowest measured value |
| Max Observed | Highest measured value |
| Mean | Average of all measured values |
| Median | Middle value (50th percentile) |
| Mode | Most frequently observed value |
| Range | Max − Min observed |
| Standard Deviation | Spread of values around the mean (lower is better) |
| Pp | Process Performance: `(USL − LSL) / (6σ)` |
| Ppk | Process Performance Index: `min((Mean − LSL) / 3σ, (USL − Mean) / 3σ)` |

Where USL/LSL are the upper/lower specification limits from the expectation's matcher
parameters (e.g., `min` and `max` for `ToBeInRange`).

!!! tip "Interpreting Pp/Ppk"
    - **Ppk ≥ 1.33** — process is capable with margin.
    - **1.0 ≤ Ppk < 1.33** — process is capable but tight.
    - **Ppk < 1.0** — process is not capable; failures are expected.
    - **Ppk < 0** — the process mean is outside the specification limits.

### Per Expectation (Exact Values)

For expectations with exact matchers (`ToBeTrue`, `ToBeFalse`, `ToBeEqual`, `ToBeType`):

| Statistic | Description |
|---|---|
| Observed Values | Each unique value seen, with its occurrence count |
| Pass rate | How often the expected value was observed |

---

## Histogram

For numeric expectations, an interactive histogram is rendered using ImPlot:

- **X axis** — measured values
- **Y axis** — frequency (or density/cumulative, configurable)
- **Vertical lines** — mark the specification limits (min/max)

### Histogram Settings

| Option | Description |
|---|---|
| Bin method | Sqrt, Sturges, Rice, Scott, or manual N bins |
| Horizontal | Flip the histogram orientation |
| Density | Normalize to probability density |
| Cumulative | Show cumulative distribution |
| No Outliers | Exclude outliers from bin calculation |

---

## Saving and Loading Reports

### Save

After generating an analysis, click **"Save Report"** to export the results as a JSON file.
This preserves the statistical summary for archival or sharing.

### Load

Click **"Load Reports"** to open previously saved analysis files. Multiple files can be loaded
simultaneously for comparison.

---

## Results Hierarchy

The analysis results are displayed in a tree structure:

```
Location: UUT1
├── Overall: 95.2% pass (142/150), avg 4.2s
├── Power On (sequence)
│   ├── 100% pass, avg 0.8s
│   ├── Supply Voltage (test)
│   │   ├── 100% pass, avg 0.3s
│   │   └── VCC (expectation)
│   │       ├── Stats: mean=3.31, σ=0.02, Ppk=1.8
│   │       └── [Histogram]
│   └── Current Draw (test)
│       └── ...
└── Functional (sequence)
    └── ...
```

---

## Tips

- **Run the analyzer after a production batch.** After testing 50+ boards, the statistics
  become meaningful. Running it on 2-3 boards won't give useful Pp/Ppk values.
- **Watch for Ppk < 1.33.** Even if everything is passing now, a low Ppk means you're at risk
  of future failures as the process drifts.
- **Use filters for focused analysis.** If a specific test is failing intermittently, filter to
  just that test to see its value distribution clearly.
- **Compare locations.** Uncheck "Combine all locations" to see if one UUT slot performs
  differently — this can indicate a fixture wiring issue.
- **Save reports before fixture changes.** Save an analysis before and after a fixture
  modification to compare process capability.

---

## See Also

- [Result Viewer](result-viewer.md) — single-run pass/fail results
- [Expectations](../lua-reference/expectations.md) — how matchers define specification limits
- [Test Lifecycle](../architecture/test-lifecycle.md) — where report files are stored
