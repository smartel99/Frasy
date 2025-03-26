--- @file    validation.lua
--- @author  Paul Thomas
--- @date    2023-03-15
--- @brief
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

local ExpectationResult = require("lua/core/framework/expectation/result")
local Expectation       = { result = nil }
Expectation.__index     = Expectation

function Expectation:New(value, name, extra)
    return setmetatable({ result = ExpectationResult:New(value, name, extra) }, Expectation)
end

function Expectation:Mandatory() return self end
function Expectation:Not() return self end
function Expectation:ToBeTrue() return self end
function Expectation:ToBeFalse() return self end
function Expectation:ToBeEqual() return self end
function Expectation:ToBeNear() return self end
function Expectation:ToBeInRange() return self end
function Expectation:ToBeInPercentage() return self end

function Expectation:ToBeGreater() return self end
function Expectation:ToBeGreaterOrEqual() return self end
function Expectation:ToBeLesser() return self end
function Expectation:ToBeLesserOrEqual() return self end

function Expectation:ToBeType() return self end
function Expectation:ToMatch() return self end
function Expectation:ExportAs(name) Orchestrator.SetValue(Orchestrator.GetScope(), name, self.result.value) end
function Expectation:Show() end

return Expectation