# Headless Mode

Frasy can run tests without a GUI using two headless execution modes:

- **CLI mode** (`--headless`) — run tests from the command line with progress output and stdin-based popup interaction
- **MCP server mode** (`--mcp-server`) — run as a [Model Context Protocol](https://modelcontextprotocol.io) tool server for AI agent integration

---

## CLI Mode

### Basic Usage

```bash
frasy.exe --headless --product MyProduct --operator "John" --serial SN001
```

Multiple UUTs:

```bash
frasy.exe --headless --product MyProduct --operator "John" --serial SN001 --serial SN002
```

### CLI Flags

| Flag | Description | Default |
|---|---|---|
| `--headless` | Enable headless CLI mode | — |
| `--mcp-server` | Run as MCP tool server (stdio JSON-RPC) | — |
| `--product <name>` | Product to test (required) | — |
| `--operator <name>` | Operator name (required) | — |
| `--serial <sn>` | Serial number per UUT (repeatable, required) | — |
| `--config <path>` | Config file path | `config.json` |
| `--output-format <fmt>` | Output format: `human` or `json` | `human` |
| `--output-dir <path>` | Output directory for reports | `logs` |
| `--skip-verification` | Skip hash verification stage | false |
| `--popup-timeout <secs>` | Auto-cancel popups after N seconds (0 = wait forever) | `0` |
| `--verbose` | Show logs on stderr | false |
| `--help` | Show usage and exit | — |

!!! note
    `--headless` and `--mcp-server` are mutually exclusive.

### Exit Codes

| Code | Meaning |
|---|---|
| `0` | All UUTs passed |
| `1` | One or more UUTs failed (expectation failures) |
| `2` | Error (setup failure, invalid arguments, missing product, etc.) |

### Output Formats

#### Human-Readable (default)

Progress is displayed as a colored indented tree:

```
[10:34:56.001] Starting: product="MyProduct" uuts=1
[10:34:56.050] [UUT1] >> Power On
[10:34:56.051] [UUT1]   > Power On > Check Voltage
[10:34:56.052] [UUT1]     [PASS] Supply Voltage
[10:34:56.052] [UUT1]   [PASS] Power On > Check Voltage
[10:34:56.053] [UUT1] [PASS] << Power On

==================================================
 Test Results: MyProduct
==================================================
 [PASS] UUT1 (SN001): PASS    [5/5 tests, 0.01s]
--------------------------------------------------
 Overall: PASS
 Reports: logs/last/1.json
==================================================
```

Colors: green for PASS, red for FAIL, white for start events.

#### JSON Lines (`--output-format json`)

Each event is a newline-delimited JSON object:

```json
{"type":"run_start","product":"MyProduct","serials":["SN001"],"uuts":1,"timestamp":"10:34:56.001"}
{"type":"sequence_start","uut":1,"serial":"SN001","sequence":"Power On","timestamp":"..."}
{"type":"test_start","uut":1,"serial":"SN001","sequence":"Power On","test":"Check Voltage","timestamp":"..."}
{"type":"expectation","uut":1,"serial":"SN001","sequence":"Power On","test":"Check Voltage","name":"Supply Voltage","pass":true,"timestamp":"..."}
{"type":"test_end","uut":1,"serial":"SN001","sequence":"Power On","test":"Check Voltage","pass":true,"timestamp":"..."}
{"type":"sequence_end","uut":1,"serial":"SN001","sequence":"Power On","pass":true,"timestamp":"..."}
{"type":"run_end","overall_pass":true,"product":"MyProduct","uuts":[{"uut":1,"serial":"SN001","pass":true,"state":"PASS","tests_passed":5,"tests_total":5,"duration":0.01,"report":"logs/last/1.json"}]}
```

---

## Popup Interaction (CLI Mode)

When a test triggers a popup in headless mode, it's presented on stdout and the process waits for input on stdin.

### Human Format

```
==================================================
 POPUP: "Confirm Fixture" (UUT 1)
--------------------------------------------------
 Place the board in the fixture
 Ensure all pins are seated

 Inputs:
   [1] Serial Number: _
   [2] Batch Code: _

 Buttons: [OK] [Cancel]
==================================================
Action>
```

**Commands:**

| Input | Effect |
|---|---|
| `<number>=<value>` | Set input field (e.g., `1=SN12345`) |
| `<ButtonLabel>` | Press a button (case-insensitive) |
| `?` | Re-display the popup |

**Example interaction:**

```
Action> 1=SN12345
  [OK] Input 1 (Serial Number) = "SN12345"
Action> 2=BATCH001
  [OK] Input 2 (Batch Code) = "BATCH001"
Action> OK
  -> Popup consumed.
```

### JSON Format

The popup is emitted as a JSON object:

```json
{"type":"popup","id":"popup_UUT1_Confirm Fixture","uut":1,"name":"Confirm Fixture","texts":["Place the board..."],"inputs":[{"index":1,"title":"Serial Number","value":""}],"buttons":["OK","Cancel"]}
```

Respond with a single JSON line on stdin:

```json
{"id":"popup_UUT1_Confirm Fixture","inputs":{"1":"SN12345","2":"BATCH001"},"button":"OK"}
```

### Timeout

With `--popup-timeout 30`, popups auto-cancel after 30 seconds if no response is received.

---

## MCP Server Mode

### Overview

With `--mcp-server`, Frasy runs as a [Model Context Protocol](https://modelcontextprotocol.io) stdio server. AI agents (via Kiro or any MCP client) can discover products, launch tests, interact with popups, and retrieve results using native tool calls.

### Kiro Agent Configuration

```json
{
  "mcpServers": {
    "frasy": {
      "command": "cmd",
      "args": ["/c", "cd /d C:\\path\\to\\bin && frasy.exe --mcp-server"]
    }
  }
}
```

### Available Tools

| Tool | Description |
|---|---|
| `list_products` | List available test products |
| `run_tests` | Start a test run (async) |
| `get_status` | Get current execution state and per-UUT states |
| `get_pending_popup` | Get the next popup waiting for interaction |
| `respond_to_popup` | Send inputs and press a button on a pending popup |
| `get_results` | Get test results summary after completion |

### Workflow

1. Call `list_products` to see available products
2. Call `run_tests` with product, operator, and serials
3. Poll `get_status` until state is `"passed"`, `"failed"`, or `"error"`
4. If `get_pending_popup` returns a popup, read it and call `respond_to_popup`
5. Call `get_results` for the full summary

### Example: run_tests

```json
// Request
{"name": "run_tests", "arguments": {"product": "MyProduct", "operator": "AI", "serials": ["SN001"], "skip_verification": true}}

// Response
{"started": true}
```

### Example: get_pending_popup + respond_to_popup

```json
// get_pending_popup response
{"id": "popup_UUT1_Enter Value", "name": "Enter Value", "uut": 1, "texts": ["Enter serial:"], "inputs": [{"index": 1, "title": "Serial", "value": ""}], "buttons": ["Submit"]}

// respond_to_popup request
{"name": "respond_to_popup", "arguments": {"id": "popup_UUT1_Enter Value", "inputs": {"1": "SN12345"}, "button": "Submit"}}
```

---

## Reports

Test reports are saved to disk regardless of mode:

- `logs/last/{uut}.json` — always overwritten with the latest run
- `logs/{product}/pass/` — passed reports (timestamped)
- `logs/{product}/fail/` — failed reports (timestamped)

Reports contain full details: timing, expectations, IB info, operator, serial.
