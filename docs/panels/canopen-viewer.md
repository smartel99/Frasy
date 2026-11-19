# CANopen Viewer

The CANopen Viewer provides a live interface for interacting with instrumentation boards on the
CAN bus. You can inspect node status, browse error histories, and manually read/write object
dictionary entries via SDO operations — invaluable for debugging hardware communication during
test development.

**Hotkey:** ++f7++

---

## Overview

The panel has two parts:

1. **Node List** — shows all registered nodes with their NMT and heartbeat status.
2. **Node Explorer** — opens when you click a node; provides tabs for errors and SDO operations.

---

## Node List

The main panel displays a table of all CANopen nodes registered in the current environment:

| Column | Description |
|---|---|
| **Name** | Board name (from `Environment.Ib.Add()`) |
| **Node ID** | CANopen node ID (hex) |
| **State** | Heartbeat state and NMT state |
| **Ping** | **Ping** button and the result of the last ping |

### Status Indicators

- **HB (Heartbeat):** `Active`, `Timeout`, `Unknown` — whether the node is sending heartbeats.
- **NMT (Network Management):** `Operational`, `Pre-operational`, `Stopped`, `Initializing`.

A node in `Operational` + `Active` state is healthy and ready for SDO communication.

### Actions

- **Click a node row** to open its Node Explorer tab.
- **"Restart" button** — resets the entire CANopen stack (all nodes re-initialize).
- **"Ping" button** (one per node) and **"Ping All"** — quickly checks that a node answers.

### Ping

CANopen has no generic ping command, so a ping reads the *Device type* (index `0x1000`), an object
that every CANopen device must have. It waits 500 ms for the answer and does not retry. The result
is shown next to the button; hover it for details (e.g., the device type that was read).

| Result | Meaning |
|---|---|
| **Responsive** | The node answered with its device type. |
| **Error** | The node answered, but with an SDO abort. The node is alive, but something is wrong. |
| **No response** | No answer before the timeout, or the request could not be sent (is CANopen running?). |

---

## Node Explorer

When you click a node, it opens in a tabbed explorer window with these tabs: Active Errors, Error
History, Object Dictionary and SDO.

### Active Errors

Shows currently active error conditions on the node. Each error displays:

- Error status bit (kind)
- Error code
- Additional info bytes

Active errors indicate ongoing problems (e.g., overcurrent, communication failure). They
clear when the condition resolves.

### Error History

Shows the history of emergency messages received from the node. Each entry includes:

- Timestamp of when the error occurred
- Error code and additional information
- Whether the error was set or cleared

This is useful for diagnosing intermittent issues that have already resolved by the time you
look.

### Object Dictionary

Browses the object dictionary of the node, so that you do not have to remember indexes. The
content comes from the **EDS or XDD file of the board** (the `ib.odPath` of the board given to `Environment.Ib.Add()`);
the format is detected automatically. The path of the file and the device it describes are shown
at the top. The file is parsed once, by whoever needs it first (the Lua environment when the product
is loaded), and the viewer shows that same result. **Reload** reads the file again after you edited it.

The objects are shown one range of indexes at a time. Use the arrows or the dropdown (like in the
analyzers) to switch between:

| Range | Content |
|---|---|
| `0x1000` - `0x1FFF` | Communication Specific Parameters |
| `0x2000` - `0x5FFF` | Manufacturer Specific Parameters |
| `0x6000` - `0x9FFF` | Device Profile Specific Parameters (shown first) |
| Other indexes | Anything else in the file (e.g., data types). Only offered when there is some. |

The dropdown shows how many objects each range has (with the filter applied).

Each object is a row, and records and arrays (e.g., `0x1018` Identity) expand to show their
sub-indexes:

| Column | Description |
|---|---|
| **Object** | Index (`0x1018`) or sub-index (`sub 0x01`). |
| **Name** | Name from the EDS/XDD. Hover for the beginning of the description (comment) written in the XDD, the data type and the default value. When there is a description, the name can be clicked to open it entirely (see below). An EDS has no descriptions. |
| **Type** | Data type of the entry. |
| **Access** | `ro`, `wo`, `rw` or `const`. |
| **Value** | The last value read from the device. |
| **New value** | Value to write (only for writable entries). Pressing ++enter++ writes it. |
| **Actions** | **Read** and **Write** buttons, and the result of the last write. |

- **Read** uploads the entry. **Read all** on a record or an array reads all its sub-indexes.
- **Write** downloads the value typed in *New value*, then reads the entry back to show what the
  device holds. Integers accept decimal or hexadecimal (`0x1F`). Entries of a type without a
  native representation (e.g., domains) are edited as hexadecimal bytes (`01 AB FF`).
- Clicking the name of an entry that has a comment opens a window with the whole text. It scrolls,
  can be resized and is closed with its close button. Several can be open at once, and they stay
  open when you select another tab.
- **Expand** and **Fold**, on the right of the filter line, expand or fold every record and array,
  in all the ranges. Typing a filter expands the records and arrays that match.
- A button is disabled when the access type does not allow the operation.
- Errors are shown in red (`Timeout`, or the SDO abort code); hover for the explanation.
- The filter keeps the objects whose index or name contains the text, in the selected range. Timeout and retries apply to
  the requests of this tab.

### SDO (Upload / Download)

The SDO tab lets you manually read from and write to any index of the node, even when it is not
in the EDS/XDD.

---

## SDO Operations

### Upload (Read)

Read a value from the node's object dictionary:

1. Enter the **Index** (hex, e.g., `0x2001`).
2. Enter the **Sub-Index** (e.g., `0`).
3. Select the **Data Type** (bool, int8, uint16, float, double, string, etc.).
4. Set **Timeout** in milliseconds (default: 1000).
5. Click **Upload**.

The result appears in the request history below, showing either the read value or an error
(abort code).

### Download (Write)

Write a value to the node's object dictionary:

1. Enter the **Index** and **Sub-Index**.
2. Select the **Data Type**.
3. Enter the **Value** to write.
4. Set **Timeout**.
5. Click **Download**.

The result (success or abort code) appears in the request history.

### Request History

Both upload and download tabs maintain a scrollable history of all requests made during the
session:

| Column | Description |
|---|---|
| Index | The OD index requested |
| Sub-Index | The sub-index |
| Result | The read value (upload) or success/failure (download) |
| Abort Code | If failed, the CANopen abort code explaining why |

Failed requests are highlighted in red. In-progress requests are highlighted in yellow.

### Options

| Option | Default | Description |
|---|---|---|
| Timeout | 1000 ms | How long to wait for a response |
| Retries | 5 | Number of retry attempts on failure |
| Block transfer | Off | Use block SDO transfer (for large data) |
| Hex input | Off | Interpret the download value as hexadecimal |

---

## Error Generator

!!! warning "Advanced / Dangerous"
    The Error Generator is a developer tool for simulating error conditions on the CANopen
    stack. It can trigger emergency messages and error states. Do not use this unless you
    understand the implications.

Access via the "Error Generator" button in the main panel. It allows you to inject specific
error kinds and codes into the CANopen stack for testing error handling logic.

---

## Common Use Cases

### Verifying Board Communication

1. Open CANopen Viewer (++f7++).
2. Check that your boards show `HB: Active, NMT: Operational`.
3. Click a node → SDO tab → upload a known entry (e.g., "Device Name" at index `0x1008`).
4. Verify the returned value matches expectations.

### Debugging a Failed SDO in Test Scripts

If a test fails with an SDO timeout or abort:

1. Open the CANopen Viewer.
2. Manually upload the same index/sub-index.
3. Check the abort code — common codes:
   - `0x06020000` — object does not exist
   - `0x06010001` — attempt to read a write-only object
   - `0x06010002` — attempt to write a read-only object
   - `0x08000020` — data transfer timeout
   - `0x06090011` — sub-index does not exist

### Manually Commanding Hardware

During development, you can manually download values to control hardware without writing test
scripts:

1. Open a node → SDO → Download tab.
2. Write to the desired OD entry (e.g., set a DAC output, toggle a relay).
3. Observe the hardware response.

---

## Tips

- **Check heartbeat first.** If HB shows `Timeout`, the node is not responding — check power,
  CAN wiring, and baud rate before trying SDO operations.
- **Use the correct data type.** SDO operations are type-sensitive. Reading a `REAL32` entry
  as `uint32` will give you the raw IEEE 754 bits, not the float value.
- **Keep the panel open during test development.** The error history captures emergencies in
  real-time, which helps correlate hardware errors with test failures.
- **Restart after bus errors.** If the CAN bus enters error-passive or bus-off state (visible
  in the Device Viewer LED indicators), use the "Restart" button to recover.

---

## See Also

- [Device Viewer](device-viewer.md) — low-level CAN frame monitoring and connection management
- [Hardware Communication](../architecture/hardware.md) — CANopen architecture, EDS files, SDO protocol
- [Custom Instrumentation Boards](../developer-guide/custom-ibs.md) — defining boards that use this bus
