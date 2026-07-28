# Popups

!!! note "Work in progress"
    This page is a placeholder. Content coming soon.

## Topics to Cover

- `Popup(name)` constructor and the builder pattern
- Elements: `:Text()`, `:TextDynamic()`, `:Input()`, `:Button()`, `:Image()`
- Layout: `:BeginHorizontal()` / `:EndHorizontal()`, `:BeginVertical()` / `:EndVertical()`, `:SameLine()`, `:Spring()`
- Displaying popups with `:Show()` (blocks the calling UUT's coroutine until consumed)
- Consuming popups: `:Consume()`, button with `consume = true` option
- `:ConsumeButtonText()` for customizing the dismiss button label
- `:Global()` popups vs. per-UUT popups
- `:Routine()` for running a function while the popup is displayed
- Reading input values from `GetInputs()`
- Initial position with the `initialPosition` parameter

## Best Practices to Cover

- When to use popups vs. pre-filling values through `Context.values` or the Control Room UI
- Designing popups that don't block teammates (interaction with teams and `Sync()`)
- Using dynamic text for progress feedback during long operations
- Layout tips: horizontal/vertical grouping, springs for clean alignment
- Keeping popup text clear and actionable for operators
- Using images to guide operators (photos of cable connections, indicator locations, etc.)
