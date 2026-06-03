--- @file    timeout_function.lua
--- @author  Paul Thomas
--- @date    2026-06-03
---
--- @copyright
--- This program is free software: you can redistribute it and/or modify it under the
--- terms of the GNU General Public License as published by the Free Software Foundation, either
--- version 3 of the License, or (at your option) any later version.
--- This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
--- even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
--- General Public License for more details.
--- You should have received a copy of the GNU General Public License along with this program. If
--- not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.

local Is = require("lua/core/utils/is")

---Waits for a condition to be met for up to duration_ms milliseconds.
---
---Sleeps for sleep_ms milliseconds between each calls to routine.
---If not provided, sleep_ms is set to 10ms.
---@param routine function A function that returns true when the condition has been met.
---@param duration_ms integer The maximum amount of time to wait for the condition.
---@param sleep_ms integer? The amount of time to wait between each calls to routine.
return function(routine, duration_ms, sleep_ms)
    CheckField(routine, Is.Function)
    CheckField(duration_ms, Is.Unsigned)
    sleep_ms = sleep_ms or 10
    CheckField(sleep_ms, Is.Unsigned)
    local deadline = duration_ms
    while (routine()) do
        SleepFor(sleep_ms)
        deadline = deadline - sleep_ms
        -- TODO we should do something better than just throw an error here...
        if deadline <= 0 then
            if Context.info.stage == Stage.execution then
                error("Timeout")
            else
                return
            end
        end
    end
end
