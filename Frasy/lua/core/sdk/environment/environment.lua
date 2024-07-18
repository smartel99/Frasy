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
local _worker = require("lua/core/sdk/environment/worker")
local _team = require("lua/core/sdk/environment/team")
local _ib = require("lua/core/sdk/environment/ib")

if (Context.map == nil) then
    Context.map = { uuts = {}, ibs = {}, team = {}, values = {}, onReport = function(report) return report end }
end

if (Ibs == nil) then Ibs = {} end

--- Environment
--- @param func function Function that defines the environment.
local function MakeEnvironment(func)
    func()
    -- _map.validate()
    _team.Validate()
    _worker.Evaluate()
end

function TestPoint(ibs, index) return { ibs = ibs, index = index } end

local function SetUutCount(count)
    Context.map.uuts = {};
    for i = 1, count do table.insert(Context.map.uuts, i) end
end

local function AddTeam(...)
    Context.team.hasTeam = true
    local leader
    for index, uuts in ipairs({ ... }) do
        if index == 1 then leader = uuts end
        assert(Context.team.players[uuts] == nil, TeamError(
            string.format("Player %d is already in team %d", uuts, leader)))
        Context.team.players[uuts] = { leader = leader, position = index }
    end
    Context.team.teams[leader] = { ... }
end

local function AddUutValue(key)
    --     assert(Context.info.stage == Stage.generation, "AddUutValue, invalid stage")
    assert(type(key) == "string", "AddUutValue, argument is not a string")
    Context.map.values[key] = {}
    local t = {}
    t.Link = function(uuts, value)
        assert(type(uuts) == "number", "AddUutValue.Link, uuts is not a number")
        Context.map.values[key][uuts] = value
        return t
    end
    return t
end

local function AddIb(board)
    if (board == nil or board.ib == nil) then
        error("Invalid board: " .. tostring(board))
    end
    local odParser = require("lua.core.can_open.object_dictionary")
    assert(Context.map.ibs[board.ib.name] == nil,
        "Ib already defined. " .. board.ib.name)
    Context.map.ibs[board.ib.name] = board
    board.ib.od = odParser.LoadFile(board.ib.eds)
    return board
end

local function SetOnReport(fun)
    Context.map.onReport = fun
end

Environment = {
    Make = MakeEnvironment,
    Uut = { Count = SetUutCount },
    Ib = { Add = AddIb },
    UutValue = { Add = AddUutValue },
    Team = { Add = AddTeam },
    Worker = { Limit = _worker.Limit },
    SetOnReport = SetOnReport,
}
