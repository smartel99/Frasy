# Proposal: Serial Number Input and Validation

**Status:** Draft  
**Date:** 2026-08-08

---

## Problem

Projects have widely different serial number input flows:

- Simple: one text field per UUT
- Range: two endpoints derive N serials
- Barcode: scan position, then scan UUT barcode, extract model from code
- Multi-step: operator selects position first, popup shows image of where to scan, then accepts input

## Solution

The framework provides N serial text fields as the default. Products override the serial input section in `ui.lua` using ImGui bindings for full control over the input UX. Validation is handled separately in `environment.lua`.

---

## Default Behavior (no override)

The framework renders N text fields labeled "Serial 1" through "Serial N", where N comes from `Environment.Uut.Count`. The run button is disabled until all enabled UUTs have a non-empty serial.

---

## Validation (Declarative)

```lua
-- In environment.lua
Environment.Make(function()
    Environment.Uut.Count(4)
    Environment.Validation.Serial("[A-Z]{2}%d+")  -- Lua pattern applied to all serials
end)
```

For more complex validation (parsing, transformation, cross-field checks), use `OnPreLaunch`:

```lua
Environment.Hooks.OnPreLaunch(function()
    for i, serial in ipairs(Context.serials) do
        if not serial:match("^[A-Z]{2}%d+$") then
            return false, "Invalid serial format for UUT " .. i .. ": " .. serial
        end
        Context.values.ui.models[i] = parseModel(serial)
    end
    return true
end)
```

---

## Custom Serial Input (UI Override)

In `ui.lua`, products override the serial section using `UI.SerialInput` with direct ImGui calls. The function is called every frame (like any ImGui rendering code) and must write serials into the provided table:

### Example: Single field, same serial for all UUTs

```lua
UI.SerialInput(function(serials, uut_count)
    local changed, value = ImGui.InputText("Serial Number", serials.input or "")
    if changed then
        serials.input = value
        for i = 1, uut_count do
            serials[i] = value
        end
    end
end)
```

### Example: Range — two fields derive N serials

```lua
UI.SerialInput(function(serials, uut_count)
    serials.first = serials.first or ""
    serials.last = serials.last or ""

    local changed
    changed, serials.first = ImGui.InputText("First Serial (Top Left)", serials.first)
    changed, serials.last = ImGui.InputText("Last Serial (Bottom Right)", serials.last)

    if #serials.first > 0 and #serials.last > 0 then
        local derived = deriveRange(serials.first, serials.last, uut_count)
        if derived then
            for i = 1, uut_count do
                serials[i] = derived[i]
            end
        end
    end
end)
```

### Example: Position-based barcode scanning with images

```lua
UI.SerialInput(function(serials, uut_count)
    for i = 1, uut_count do
        ImGui.PushID(i)

        -- UUT button shows status
        local texture = UI.GetUutTexture(i)
        if ImGui.ImageButton(texture, 80, 80) then
            serials.active_uut = i
        end
        ImGui.SameLine()
        ImGui.BeginGroup()
        ImGui.Text("UUT " .. i)
        ImGui.Text("Serial: " .. (serials[i] or ""))
        ImGui.Text("Model: " .. (serials.models and serials.models[i] or ""))
        ImGui.EndGroup()

        ImGui.PopID()
    end

    -- Scanning popup for selected position
    if serials.active_uut then
        local uut = serials.active_uut
        local open = true
        open, _ = ImGui.Begin("Scan UUT " .. uut)
        if open then
            ImGui.Image("assets/uut_location_" .. uut .. ".png")
            ImGui.SetKeyboardFocusHere()
            local changed, value = ImGui.InputText("Barcode", serials.scan_buffer or "")
            if changed then
                serials.scan_buffer = value
                if isBarcodeComplete(value) then
                    serials[uut] = value
                    serials.models = serials.models or {}
                    serials.models[uut] = parseModel(value)
                    serials.scan_buffer = ""
                    serials.active_uut = nil
                end
            end
        end
        ImGui.End()
    end
end)
```

---

## How It Works

- `UI.SerialInput(fn)` replaces the default serial text fields in the control room
- The function receives a `serials` table (persistent across frames) and the UUT count
- The function writes serials into `serials[1]` through `serials[N]`
- The framework reads those values when the operator presses Run
- The function can use any state it wants by storing it on the `serials` table (it persists across frames)
- The function can use any ImGui call available in the Lua bindings

---

## Headless Mode Behavior

In headless mode:

- `UI.SerialInput` is **ignored** — serials come from `--serial` flags or MCP `run_tests` arguments
- `Environment.Validation.Serial` still applies to CLI-provided serials
- `Environment.Hooks.OnPreLaunch` still runs for final validation/transformation

This means products work in both modes without changes — the custom input is purely a GUI concern.

---

## Validation Order

### GUI mode

```
1. UI.SerialInput() populates serials each frame (or default fields)
2. Operator presses Run
3. Framework checks all enabled UUTs have non-empty serials
4. Environment.Validation.Serial(pattern) checks format
5. Environment.Hooks.OnPreLaunch() — final validation/transformation
6. Orchestrator starts
```

### Headless mode

```
1. --serial flags provide serials
2. Framework checks correct number of serials provided
3. Environment.Validation.Serial(pattern) checks format
4. Environment.Hooks.OnPreLaunch() — final validation/transformation
5. Orchestrator starts
```

---

## Context Access

Serials and UI values are available throughout the test lifecycle:

```lua
Context.serials[uut]          -- the serial for this UUT
Context.values.ui["field"]    -- extra UI fields declared with UI.Field
Context.info.operator         -- operator name
```
