# Popups

Popups are modal dialog windows displayed to the operator during test execution. They are used
to request manual actions (connect a cable, verify an LED color), collect input (enter a serial
number), or display progress. A popup **blocks** the calling UUT's execution until it is
dismissed (consumed).

---

## `Popup(name, initialPosition?)`

Creates a popup builder. Chain element methods to add content, then call `:Show()` to display.

```lua
Popup("Connect Cable")
    :Text("Connect the USB cable to port J1")
    :Image("lua/user/my_product/assets/j1_photo.jpg")
    :Show()
```

**Parameters:**

| Name | Type | Description |
|---|---|---|
| `name` | `string?` | Title displayed in the popup window (defaults to `""`) |
| `initialPosition` | `{number, number}?` | Initial window position `{x, y}` in pixels |

**Returns:** A `PopupBuilder` object.

---

## Elements

Elements are added to the popup via chainable builder methods. They render in the order they
are added.

### `:Text(text)`

Displays a static text label.

```lua
Popup("Info"):Text("Please wait for the board to initialize"):Show()
```

| Parameter | Type | Description |
|---|---|---|
| `text` | `string` | The text to display |

---

### `:TextDynamic(routine)`

Displays text that updates every frame. The routine is called repeatedly and must return a
string.

```lua
local startTime = os.clock()
Popup("Progress")
    :TextDynamic(function()
        return string.format("Elapsed: %.1fs", os.clock() - startTime)
    end)
    :Show()
```

| Parameter | Type | Description |
|---|---|---|
| `routine` | `function() → string` | Called every frame; return value becomes the displayed text |

---

### `:Input(title)`

Displays a text input field. The operator can type into it.

```lua
Popup("Serial Number")
    :Text("Scan or type the board serial number:")
    :Input("SN")
    :Button("OK", function() end, { consume = true })
    :Show()
```

| Parameter | Type | Description |
|---|---|---|
| `title` | `string` | Label shown next to the input field |

Input values are collected in order and passed to button actions and routines (see
[Reading Input Values](#reading-input-values)).

---

### `:Button(label, action, opt?)`

Displays a clickable button.

```lua
Popup("Confirm")
    :Text("Is the LED green?")
    :Button("Yes", function() end, { consume = true })
    :SameLine()
    :Button("No", function() end, { consume = true })
    :Show()
```

| Parameter | Type | Description |
|---|---|---|
| `label` | `string` | Button text |
| `action` | `function(inputs)` | Called when clicked. Receives the current input values as a table. |
| `opt.size` | `{width, height}?` | `{0, 0}` (auto) | Button size in pixels |
| `opt.consume` | `boolean?` | `false` | If `true`, clicking this button dismisses the popup |


---

### `:Image(path, size?)`

Displays an image loaded from a file path.

```lua
Popup("Assembly Guide")
    :Text("Attach the connector as shown:")
    :Image("lua/user/my_product/assets/connector.png", { width = 300, height = 200 })
    :Show()
```

| Parameter | Type | Description |
|---|---|---|
| `path` | `string` | Path to the image file (relative to the executable directory) |
| `size` | `{width, height}?` | Display size in pixels (defaults to the image's native size) |

---

## Layout

By default, elements stack vertically. Use layout methods to arrange elements horizontally,
add spacing, or create complex arrangements.

### `:SameLine(opt?)`

Places the next element on the same line as the previous one.

```lua
Popup("Choice")
    :Button("Accept", function() end, { consume = true })
    :SameLine({ spacing = 20 })
    :Button("Reject", function() end, { consume = true })
    :Show()
```

| Parameter | Type | Default | Description |
|---|---|---|---|
| `opt.offsetFromStartX` | `number?` | `0.0` | Horizontal offset from the window's left edge |
| `opt.spacing` | `number?` | `-1.0` (auto) | Spacing between elements |

---

### `:BeginHorizontal(id, opt?)` / `:EndHorizontal()`

Groups elements in a horizontal layout container.

```lua
Popup("Panel")
    :BeginHorizontal(1)
        :Button("Left", function() end)
        :Spring()
        :Button("Right", function() end)
    :EndHorizontal()
    :Show()
```

| Parameter | Type | Description |
|---|---|---|
| `id` | `integer` | Unique ID for this layout group |
| `opt.size` | `{width, height}?` | Container size (default auto) |
| `opt.align` | `number?` | Alignment (-1.0 = default) |

---

### `:BeginVertical(id, opt?)` / `:EndVertical()`

Groups elements in a vertical layout container. Useful inside a horizontal group to create
columns.

```lua
Popup("Layout Example")
    :BeginHorizontal(1)
        :BeginVertical(2)
            :Button("Top-Left", function() end)
            :Button("Bottom-Left", function() end)
        :EndVertical()
        :Image("assets/textures/icon.png", { width = 100, height = 100 })
    :EndHorizontal()
    :Show()
```

| Parameter | Type | Description |
|---|---|---|
| `id` | `integer` | Unique ID for this layout group |
| `opt.size` | `{width, height}?` | Container size (default auto) |
| `opt.align` | `number?` | Alignment (-1.0 = default) |

---

### `:Spring(opt?)`

Inserts flexible space inside a horizontal or vertical group. Springs push surrounding elements
apart.

```lua
:BeginHorizontal(1)
    :Button("Left", function() end)
    :Spring()  -- pushes "Right" to the far right
    :Button("Right", function() end)
:EndHorizontal()
```

| Parameter | Type | Default | Description |
|---|---|---|---|
| `opt.weight` | `number?` | `1` | Relative weight (higher = more space) |
| `opt.spacing` | `number?` | `-1` (auto) | Fixed spacing override |

---

## Displaying and Dismissing

### `:Show()`

Displays the popup and **blocks** the calling UUT's execution until the popup is consumed.

```lua
Popup("Wait"):Text("Press OK when ready"):Show()
-- Execution resumes here after the operator dismisses the popup
```

**Notes:**

- Only active during execution. During generation/validation, `:Show()` is a no-op.
- The popup window is rendered by the UI thread; the Lua thread sleeps until consumption.
- If no button has `consume = true`, a default dismiss button is shown automatically (labeled
  "Cancel" unless overridden with `:ConsumeButtonText()`).

### `:Consume()`

Programmatically dismisses the popup from within a `:Routine()` or externally.

```lua
local popup = Popup("Auto-Close")
popup:Text("Waiting for sensor...")
popup:Routine(function()
    if sensorReady() then popup:Consume() end
end)
popup:Show()
```

### `:ConsumeButtonText(text)`

Changes the label of the default dismiss button (shown when no button has `consume = true`).

```lua
Popup("Confirmation")
    :Text("Board is ready for the next step")
    :ConsumeButtonText("Continue")
    :Show()
```

| Parameter | Type | Default | Description |
|---|---|---|---|
| `text` | `string` | `"Cancel"` | Label for the auto-generated dismiss button |

---

## Global vs. Per-UUT Popups

### `:Global()`

By default, popups are **per-UUT** — their window title is prefixed with the UUT number
(e.g., "UUT1 - Connect Cable"). This distinguishes popups when multiple UUTs run
simultaneously.

Call `:Global()` to make the popup shared — no UUT prefix is added:

```lua
Popup("Shared Instruction")
    :Global()
    :Text("Apply power to the fixture")
    :Show()
```

Use `:Global()` when the instruction applies to the entire fixture rather than a specific UUT.

---

## Routines

### `:Routine(routine)`

Registers a function that runs in a loop while the popup is displayed. The routine is called
repeatedly (approximately every 1ms) until the popup is consumed.

```lua
local popup = Popup("Lid Check")
popup:Text("Close the lid")
popup:Routine(function()
    if isLidClosed() then popup:Consume() end
end)
popup:Show()
```

| Parameter | Type | Description |
|---|---|---|
| `routine` | `function(inputs)` | Called repeatedly. Receives current input values. |

**Common uses:**

- Auto-dismiss when a hardware condition is met (sensor triggered, lid closed)
- Timeout-based auto-dismiss
- Polling for external events

---

## Reading Input Values

When a popup contains `:Input()` fields, their values are accessible through:

1. **Button actions** — the `action` function receives the inputs table as its first argument.
2. **Routines** — the routine function receives the inputs table as its first argument.

Inputs are indexed in the order they were added (1-indexed):

```lua
Popup("Enter Values")
    :Input("Lot Number")     -- inputs[1]
    :Input("Operator ID")    -- inputs[2]
    :Button("Submit", function(inputs)
        Log.I("Lot: " .. inputs[1])
        Log.I("Operator: " .. inputs[2])
    end, { consume = true })
    :Show()
```

---

## Complete Examples

### Simple Confirmation

```lua
Popup("Step Complete")
    :Text("Remove the test cable from J5")
    :ConsumeButtonText("Done")
    :Show()
```

### Visual Inspection with Pass/Fail

```lua
local function LedCheck(color)
    local passed = false
    Popup("LED Check")
        :Text("Verify that LED D500 is " .. color)
        :Image("lua/user/my_product/assets/led_" .. color .. ".jpg")
        :Button("PASS", function() passed = true end, { consume = true })
        :SameLine({ spacing = 100 })
        :Button("FAIL", function() end, { consume = true })
        :Show()
    return passed
end

Test("LED Color", function()
    enableRedLed()
    Expect(LedCheck("red"), "LED is Red"):ToBeTrue()
end)
```

### Auto-Dismiss on Hardware Event

```lua
Test("Close Lid", function()
    local popup = Popup("Lid")
    popup:Text("Close the fixture lid and press firmly")
    popup:Routine(function()
        if readLidSensor() then popup:Consume() end
    end)
    popup:Show()
    Expect(readLidSensor(), "Lid Closed"):ToBeTrue()
end)
```

### Timed Auto-Dismiss

```lua
Test("Stabilization", function()
    local start = os.clock()
    local popup = Popup("Please Wait")
    popup:Text("Waiting for temperature stabilization...")
    popup:TextDynamic(function()
        local elapsed = os.clock() - start
        return string.format("%.0f / 30 seconds", elapsed)
    end)
    popup:Routine(function()
        if os.clock() - start >= 30 then popup:Consume() end
    end)
    popup:Show()
end)
```

### Collecting Operator Input

```lua
Test("Label Scan", function()
    Popup("Scan Label")
        :Text("Scan the barcode on the board label:")
        :Input("Barcode")
        :Button("Submit", function(inputs)
            Context.values.scanned_barcode = inputs[1]
        end, { consume = true })
        :Show()

    Expect(Context.values.scanned_barcode, "Barcode Scanned")
        :Not():ToBeEqual("")
end)
```

---

## Best Practices

- **Prefer automation over popups.** Every popup pauses test execution and requires operator
  attention. If a check can be done with a sensor (lid switch, optical detector), automate it
  instead.
- **Keep text short and actionable.** Tell the operator exactly what to do: "Connect cable to
  J5" not "Please ensure the cable is properly connected to the designated port."
- **Use images for visual inspections.** A photo showing the expected state eliminates
  ambiguity and reduces errors.
- **Beware of team deadlocks.** Popups block the calling UUT. If teams are enabled, all team
  members sync at every test boundary. A popup that blocks one member indefinitely will block
  the entire team. Use `:Global()` or place popups in sequences where only the leader shows
  them.
- **Use `:Routine()` for auto-dismiss.** When waiting for a physical action (lid close, button
  press), poll the sensor in a routine rather than making the operator click a button. This
  is faster and less error-prone.
- **Use `:TextDynamic()` for feedback.** Show progress (elapsed time, sensor readings) so the
  operator knows the system is responsive.
- **Use `Once()` with global popups.** If a popup should appear once for the entire fixture
  (not once per UUT), wrap it in `Once()`:

```lua
Once(function()
    Popup("Fixture Setup")
        :Global()
        :Text("Ensure all boards are seated")
        :ConsumeButtonText("Ready")
        :Show()
end)
```

---

## API Summary

| Method | Description |
|---|---|
| `Popup(name, pos?)` | Create a popup builder |
| `:Text(text)` | Add a static text label |
| `:TextDynamic(fn)` | Add a dynamically-updating text label |
| `:Input(title)` | Add a text input field |
| `:Button(label, action, opt?)` | Add a button (optionally consuming) |
| `:Image(path, size?)` | Add an image |
| `:SameLine(opt?)` | Place next element on the same line |
| `:BeginHorizontal(id, opt?)` | Start horizontal layout group |
| `:EndHorizontal()` | End horizontal layout group |
| `:BeginVertical(id, opt?)` | Start vertical layout group |
| `:EndVertical()` | End vertical layout group |
| `:Spring(opt?)` | Insert flexible space |
| `:Global()` | Remove UUT prefix from the window title |
| `:Routine(fn)` | Register a function that loops while popup is shown |
| `:ConsumeButtonText(text)` | Customize the default dismiss button label |
| `:Show()` | Display the popup (blocks until consumed) |
| `:Consume()` | Programmatically dismiss the popup |

---

## See Also

- [Sequences & Tests](sequences-and-tests.md) — `Once()` and `Exclusive()` for multi-UUT coordination
- [Teams](../developer-guide/teams.md) — how popups interact with team synchronization
- [Context](context.md) — storing popup-collected values for use in expectations
