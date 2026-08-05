--- @file    environment.lua
--- @brief   CLI-tests product environment — a minimal product for headless testing.

Environment.Make(function()
    Environment.Uut.Count(1)
    Environment.ScriptVersion("1.0.0")
end)
