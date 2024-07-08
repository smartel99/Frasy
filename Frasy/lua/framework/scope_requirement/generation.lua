--- @file    generation.lua
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

local OrderRequirement   = require("lua/core/framework/order_requirement")
local RuntimeRequirement = require("lua/core/framework/runtime_requirement")

local ScopeRequirement   = {
    orchestrator = nil,
    scope        = nil,
}
ScopeRequirement.__index = ScopeRequirement

function ScopeRequirement:New(orchestrator, scope)
    return setmetatable({ orchestrator = orchestrator, scope = scope }, ScopeRequirement)
end

function ScopeRequirement:Test(name)
    self.scope.test = name
    -- OrderRequirement can happen anytime
    -- Thus, we must allow requesting unknown scope
    --if not self.orchestrator.HasTest(self.scope) then error(NotFound()) end
    return self
end

function ScopeRequirement:Value(name)
    return self.orchestrator.GetValue(self.scope, name)
end

function ScopeRequirement:ToBeFirst()
    self.orchestrator.AddOrderRequirement(OrderRequirement:New(self.scope, nil, OrderRequirement.Kind.first))
    return RuntimeRequirement:New(function() return true end)
end

function ScopeRequirement:ToBeLast()
    self.orchestrator.AddOrderRequirement(OrderRequirement:New(self.scope, nil, OrderRequirement.Kind.last))
    return RuntimeRequirement:New(function() return true end)
end

function ScopeRequirement:ToBeBefore()
    self.orchestrator.AddOrderRequirement(OrderRequirement:New(self.orchestrator.GetScope(), self.scope,
                                                               OrderRequirement.Kind.after))
    return RuntimeRequirement:New(function() return true end)
end

function ScopeRequirement:ToBeAfter()
    self.orchestrator.AddOrderRequirement(OrderRequirement:New(self.scope, self.orchestrator.GetScope(),
                                                               OrderRequirement.Kind.after))
    return RuntimeRequirement:New(function() return true end)
end

function ScopeRequirement:ToBeRightBefore()
    self.orchestrator.AddOrderRequirement(OrderRequirement:New(self.orchestrator.GetScope(), self.scope,
                                                               OrderRequirement.Kind.after))
    return RuntimeRequirement:New(function() return true end)
end

function ScopeRequirement:ToBeRightAfter()
    self.orchestrator.AddOrderRequirement(OrderRequirement:New(self.scope, self.orchestrator.GetScope(),
                                                               OrderRequirement.Kind.after))
    return RuntimeRequirement:New(function() return true end)
end

function ScopeRequirement:ToPass()
    -- Cannot perform multiple tests for same board for now
    -- Thus, the current test must always be after the requested scope
    self.orchestrator.AddOrderRequirement(OrderRequirement:New(self.orchestrator.GetScope(), self.scope,
                                                               OrderRequirement.Kind.after))
    return RuntimeRequirement:New(function() return true end)
end

function ScopeRequirement:ToFail()
    -- Cannot perform multiple tests for same board for now
    -- Thus, the current test must always be after the requested scope
    self.orchestrator.AddOrderRequirement(OrderRequirement:New(self.orchestrator.GetScope(), self.scope,
                                                               OrderRequirement.Kind.after))
    return RuntimeRequirement:New(function() return true end)
end

function ScopeRequirement:ToBeComplete()
    -- Cannot perform multiple tests for same board for now
    -- Thus, the current test must always be after the requested scope
    self.orchestrator.AddOrderRequirement(OrderRequirement:New(self.orchestrator.GetScope(), self.scope,
                                                               OrderRequirement.Kind.after))
    return RuntimeRequirement:New(function() return true end)
end

return ScopeRequirement