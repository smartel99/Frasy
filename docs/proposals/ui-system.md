# Proposal: Declarative UI System

**Status:** Draft  
**Date:** 2026-08-08

---

## Problem

Each Frasy project currently implements its own `renderControlRoom()` in C++, duplicating 80% boilerplate (product dropdown, operator field, run button, UUT indicators) while customizing 20% (images, instructions, extra fields, debug menus).

## Solution

The single Frasy executable ships a built-in control room that handles all common UI elements. Products customize their UI through an optional `ui.lua` file in their product directory, using a combination of declarative functions for common patterns and direct ImGui calls for advanced customization.

---

## Default Behavior (no `ui.lua`)

The framework renders:

- Product dropdown (auto-detected from `lua/user/`)
- Operator name field
- N serial number fields (from `Environment.Uut.Count`)
- Run button
- UUT status indicators (pass/fail/running/disabled)
- Result viewer on completion

---

## Declarative Customization (`ui.lua`)

```lua
UI.Make(function()
    -- Static content
    UI.Image("assets/pcb.jpg")
    UI.Instructions({
        "Place UUT on fixture",
        "Connect J1 to UUT-J7 (Max 10 insertions)",
        "Ensure all pins are seated",
    })

    -- Extra input fields (available at Context.values.ui.<key>)
    UI.Field("Batch Code", { required = true })
    UI.Field("Firmware Version", { default = "1.0.0" })

    -- Serial input override (see serial-input-validation proposal)
    UI.SerialInput(...)

    -- Advanced: custom ImGui rendering
    UI.OnRender(function()
        ImGui.Text("Custom panel content")
        if ImGui.Button("Do Something") then
            MyPlugin.doSomething()
        end
    end)
end)
```

---

## Available UI Declarations

| Function | Purpose | Rendered as |
|----------|---------|-------------|
| `UI.Image(path)` | Product/PCB photo | Image panel beside controls |
| `UI.Instructions(list)` | Setup steps | Bulleted list |
| `UI.Field(name, opts)` | Extra input field | Text input with label |
| `UI.SerialInput(...)` | Override serial acquisition (see serial-input-validation proposal) | Replaces default serial fields |
| `UI.OnRender(fn)` | Custom ImGui rendering | Called every frame in a dedicated panel |

---

## Field Options

```lua
UI.Field("Model", {
    required = true,              -- block run if empty
    default = "",                 -- initial value
    validator = "^[A-Z]{2}%d+$", -- lua pattern
    choices = {"A", "B", "C"},   -- dropdown instead of text input
})
```

---

## Custom ImGui Rendering

For cases that go beyond declarative elements, products use `UI.OnRender()` with a lightweight ImGui binding exposed to Lua. This covers debug panels, custom visualizations, hardware status displays, or any arbitrary GUI element:

```lua
UI.OnRender(function()
    -- Full ImGui access for power users
    if ImGui.BeginTable("##DeviceInfo", 2) then
        for i = 1, Context.uut_count do
            ImGui.TableNextRow()
            ImGui.TableNextColumn()
            ImGui.Text("UUT " .. i)
            ImGui.TableNextColumn()
            ImGui.Text(Context.values.ui.models[i] or "N/A")
        end
        ImGui.EndTable()
    end

    -- Interact with plugins
    if ImGui.Button("Power Up Testbench") then
        MyPlugin.powerUp()
    end

    -- Debug-only sections
    if Debug then
        ImGui.Separator()
        ImGui.Text("DEBUG CONTROLS")
        Context.values.ui.skipPower = ImGui.Checkbox("Skip Power", Context.values.ui.skipPower)
    end
end)
```

---

## ImGui Lua Bindings (Subset)

The framework exposes a practical subset of ImGui to Lua:

| Category | Functions |
|----------|-----------|
| Text | `Text`, `TextColored`, `TextWrapped`, `BulletText`, `LabelText` |
| Inputs | `InputText`, `InputInt`, `InputFloat`, `Checkbox`, `SliderInt`, `SliderFloat` |
| Buttons | `Button`, `ImageButton`, `RadioButton` |
| Layout | `SameLine`, `Separator`, `Dummy`, `Spacing`, `NewLine`, `BeginGroup`/`EndGroup` |
| Tables | `BeginTable`/`EndTable`, `TableNextRow`, `TableNextColumn`, `TableSetupColumn` |
| Trees/Collapsing | `TreeNode`/`TreePop`, `CollapsingHeader` |
| Popups | `OpenPopup`, `BeginPopup`/`EndPopup`, `BeginPopupModal`/`EndPopup` |
| Windows | `Begin`/`End`, `BeginChild`/`EndChild` |
| Combo/List | `BeginCombo`/`EndCombo`, `Selectable`, `ListBox` |
| Images | `Image` (from loaded texture paths) |
| State | `IsItemClicked`, `IsItemHovered`, `GetTime` |

This is not a complete ImGui binding — it covers the controls needed for test station UIs without exposing low-level rendering internals.

---

## Accessing UI Values from Tests

```lua
-- In test scripts:
local batch = Context.values.ui["Batch Code"]
local model = Context.values.ui["Model"]
```

---

## Headless Mode Behavior

- `UI.Image` and `UI.Instructions` are ignored (no display)
- `UI.Field` values can be passed via CLI: `--field "Batch Code=ABC123"`
- `UI.SerialInput` uses headless-mode fallback (see serial-input-validation proposal)
- `UI.OnRender` is ignored

---

## Debug Fields

For debug checkboxes that only render in debug builds:

```lua
UI.Field("Skip Power Tests", { type = "bool", default = false, debug = true })
UI.Field("Skip MCU Tests", { type = "bool", default = false, debug = true })
```
