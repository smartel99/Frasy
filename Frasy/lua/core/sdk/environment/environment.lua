--- @file    environment.lua
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

local mapper = require("lua/core/sdk/environment/map")
local worker = require("lua/core/sdk/environment/worker")
local team   = require("lua/core/sdk/environment/team")

function Environment(func)
    func()
    Map = mapper.validate()
    team.validate()
    worker.evaluate()
end

function TestPoint(name)
    if name ~= nil then mapper.TestPoint.New(name) end
    return {
        To    = mapper.TestPoint.To,
        Count = mapper.TestPoint.Count,
    }
end

UUT    = { Count = mapper.UUT.Count }

IB     = { Count = mapper.IB.Count }

Worker = { Count = worker.count, Limit = worker.limit }
