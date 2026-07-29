# Device Viewer

The Device Viewer manages the connection to SLCAN USB-to-CAN adapters and displays live CAN bus
traffic. It shows which devices are connected, allows manual port selection, and provides a
real-time view of CAN frames on the network.

**Hotkey:** ++f3++

---

## Overview

The Device Viewer serves two purposes:

1. **Connection management** — detecting, connecting, and disconnecting SLCAN adapters.
2. **Network monitoring** — showing live CAN frames with their IDs, data length, and payload.

---

## Connection Status

The connection state is always visible in the **main menu bar** (top-right area):

- **Green text: "Connected (N)"** — N SLCAN devices are connected and active.
- **Red text: "Disconnected"** — no SLCAN devices are open.
- **LED indicators** — red and green dots reflect the CANopen stack's state (TX/RX activity).

Click the connection status text to open the Device Viewer panel.

---

## Connecting to a Device

### Automatic Detection

On startup, Frasy scans all USB serial ports and automatically connects to devices matching the
**USB whitelist** in `config.json`:

```json
"communication": {
    "usbWhitelist": [
        { "vid": 1155, "pid": 42180, "mi": 0 }
    ]
}
```

SMarTest devices (DAQ, PIO, R8L) are always accepted regardless of the whitelist.

### Hot-Plug Support

When a USB device matching the whitelist is connected or disconnected, the Device Viewer
detects the change automatically and connects/disconnects as appropriate.

### Manual Connection

If automatic detection doesn't work:

1. Open the Device Viewer (++f3++).
2. Select the serial port from the **Port** dropdown.
3. Click **Refresh** if the port doesn't appear.

---

## Network State

Once connected, the panel displays a live table of CAN frames seen on the bus:

| Column | Description |
|---|---|
| **Id** | CAN frame ID (hex) |
| **IsExt** | Whether this is an extended (29-bit) frame |
| **IsRTR** | Whether this is a Remote Transmission Request frame |
| **DLC** | Data Length Code (number of data bytes) |
| **Data** | Raw data bytes (hex) |

The table shows the **last seen** frame for each unique CAN ID. It updates in real-time as
packets flow.

### Statistics

Above the table, a status line shows:

- **Addresses** — number of unique CAN IDs seen
- **Pending packets** — packets queued for processing
- **Total packets** — total RX packet count since startup
- **Packets/second** — current receive throughput
- **kB/s** — current bandwidth usage

Click **Clear** to reset the network state table and counters.

---

## Menu Bar Indicators

The Device Viewer contributes network usage indicators to the main menu bar:

- **RX/TX progress bars** — color-coded from green (low usage) through yellow to red (high
  usage), showing CAN bus utilization.
- **LED dots** — reflect the CANopen stack's internal red/green LED state (similar to physical
  CANopen device LEDs indicating NMT state and errors).

---

## Configuration

The Device Viewer uses the `communication` key in `config.json`:

```json
"communication": {
    "usbWhitelist": [
        { "vid": 1155, "pid": 42180, "mi": 0 }
    ]
}
```

| Field | Type | Description |
|---|---|---|
| `usbWhitelist` | array | USB device identifiers to match for auto-connection |
| `usbWhitelist[].vid` | int | USB Vendor ID |
| `usbWhitelist[].pid` | int | USB Product ID |
| `usbWhitelist[].mi` | int? | USB Interface Number (optional) |
| `usbWhitelist[].rev` | int? | USB Revision (optional) |

To find your adapter's VID/PID, check Windows Device Manager → the device's Properties →
Details → Hardware Ids. The format is `USB\VID_XXXX&PID_XXXX&MI_XX`.

---

## Tips

- **If the device isn't detected**, verify the VID/PID/MI in Device Manager matches your
  whitelist entry in `config.json`.
- **"Pending packets" growing** indicates the CANopen stack can't process frames fast enough.
  This may happen if many nodes are transmitting simultaneously.
- **Use this panel for hardware debugging.** The raw CAN frame view helps identify
  communication issues (missing nodes, unexpected frames, wrong baud rate).
- **The panel can remain closed during normal operation.** The connection is maintained
  regardless of panel visibility — the panel is just a monitoring tool.

---

## See Also

- [Hardware Communication](../architecture/hardware.md) — SLCAN, CANopen, and SDO architecture
- [CANopen Viewer](canopen-viewer.md) — higher-level object dictionary browsing
- [Configuration](../developer-guide/configuration.md) — USB whitelist setup
