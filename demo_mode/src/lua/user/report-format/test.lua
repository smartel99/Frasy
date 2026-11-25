--- @file    test.lua
--- @author  Sam Martel
--- @date    2026-11-25
--- @brief   Sequences ending with every scope result.
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

--- Unmet only when running for real, so generation keeps the scope.
local function NotApplicable()
    return RequirementSpecifier(function() return Context.info.stage ~= Stage.execution end)
end

Sequence("Mixed", function()
    Test("Pass", function()
        Expect(2 + 2, "Addition"):ToBeEqual(4)
    end)
    Test("Skipped", function()
        Requires(NotApplicable())
        Expect(true, "Never checked"):ToBeTrue()
    end)
    -- Disable this test to get an incomplete board, saved under debug/.
    Test("Optional", function()
        Expect(3.3, "Voltage"):ToBeInRange(3.0, 3.6)
    end)
end)

Sequence("Not Applicable", function()
    Requires(NotApplicable())
    Test("Never Runs", function()
        Expect(true, "Never checked"):ToBeTrue()
    end)
end)
