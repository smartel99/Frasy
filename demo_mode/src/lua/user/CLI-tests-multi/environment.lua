--- @file    environment.lua
--- @brief   CLI-tests-multi — multi-UUT product for headless testing.

Environment.Make(function()
    Environment.Uut.Count(2)
    Environment.ScriptVersion("1.0.0")
end)
