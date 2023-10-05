local function DebugHook(event, line)
    -- Log.d("Event: " .. event .. " , line: " .. (line ~= nil and line or "nil"))
end

debug.sethook(DebugHook, 'crl')