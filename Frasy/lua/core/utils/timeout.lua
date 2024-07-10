local IsUnsigned = require("lua.core.utils.is_unsigned.is_unsigned")

return function(routine, duration_ms, sleep_ms)
    assert(type(routine) == "function", "Invalid routine: " .. tostring(routine))
    assert(IsUnsigned(duration_ms),
           "Invalid duration: " .. tostring(duration_ms))
    if sleep_ms == nil then sleep_ms = 10 end
    assert(IsUnsigned(sleep_ms), "Invalid sleep: " .. tostring(sleep_ms))

    local deadline = duration_ms
    while (routine()) do
        Utils.SleepFor(sleep_ms)
        deadline = deadline - sleep_ms
        if deadline <= 0 then error("Timeout") end
    end
end
