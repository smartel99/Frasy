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

local RuntimeRequirement = require("lua/core/framework/runtime_requirement")

local ScopeRequirement = {
    orchestrator = nil,
    scope        = nil,
}
ScopeRequirement.__index = ScopeRequirement

function ScopeRequirement:new(orchestrator, scope)
    return setmetatable({ orchestrator = orchestrator, scope = scope }, ScopeRequirement)
end

function ScopeRequirement:Test(name)
    self.scope.test = name
    if not self.orchestrator.HasTest(self.scope) then error(NotFound()) end
    return self
end

function ScopeRequirement:Value(name)
    return self.orchestrator.GetValue(self.scope, name)
end

function ScopeRequirement:ToBeFirst()
    -- Order requirement are ignore when validating
    return RuntimeRequirement:new(function() return true end)
end

function ScopeRequirement:ToBeLast()
    -- Order requirement are ignore when validating
    return RuntimeRequirement:new(function() return true end)
end

function ScopeRequirement:ToBeBefore(other)
    -- Order requirement are ignore when validating
    return RuntimeRequirement:new(function() return true end)
end

function ScopeRequirement:ToBeAfter(other)
    -- Order requirement are ignore when validating
    return RuntimeRequirement:new(function() return true end)
end

function ScopeRequirement:ToBeRightBefore(other)
    -- Order requirement are ignore when validating
    return RuntimeRequirement:new(function() return true end)
end

function ScopeRequirement:ToBeRightAfter(other)
    -- Order requirement are ignore when validating
    return RuntimeRequirement:new(function() return true end)
end

function ScopeRequirement:ToPass()
    -- Order requirement are ignore when validating
    return RuntimeRequirement:new(function() return true end)
end

function ScopeRequirement:ToFail()
    -- Order requirement are ignore when validating
    return RuntimeRequirement:new(function() return true end)
end

function ScopeRequirement:ToBeComplete()
    -- Order requirement are ignore when validating
    return RuntimeRequirement:new(function() return true end)
end

return ScopeRequirement