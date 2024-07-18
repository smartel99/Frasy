local IsUnsigned = require("lua.core.utils.is_unsigned.is_unsigned")

---Waits for a condition to be met for up to duration_ms milliseconds.
---
---Sleeps for sleep_ms milliseconds between each calls to routine.
---If not provided, sleep_ms is set to 10ms.
---@param routine function A function that returns true when the condition has been met.
---@param duration_ms integer The maximum amount of time to wait for the condition.
---@param sleep_ms integer? The amount of time to wait between each calls to routine.
return function(routine, duration_ms, sleep_ms)
    assert(type(routine) == "function", "Expected function for routine, got " .. type(routine))
    assert(IsUnsigned(duration_ms),
        "Expected integer for duration_ms, got: " .. type(duration_ms))
    if sleep_ms == nil then sleep_ms = 10 end
    assert(IsUnsigned(sleep_ms), "Expected integer for sleep_ms, got: " .. type(sleep_ms))

    local deadline = duration_ms
    while (routine()) do
        Utils.SleepFor(sleep_ms)
        deadline = deadline - sleep_ms
        -- TODO we should do something better than just throw an error here...
        if deadline <= 0 then error("Timeout") end
    end
end
