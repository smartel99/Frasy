--- @file    worker.lua
--- @author  Paul Thomas
--- @date    3/20/2023
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

local worker   = {}
local internal = {
    count = 0,
    limit = nil
}

local function __evaluate_team()
    assert(type(internal.limit.reference) == "number", WorkerError("Team limitation must be a number"))
    local stage = {}
    for team in pairs(Context.team.teams) do

    end
end

local function __evaluate_ib()
    assert(internal.limit.reference == Team, WorkerError("Ib limitation must be linked to Team"))
    for player in pairs(Context.team.players) do
        local playerIb    = Context.map.ibs[player]
        local playerTeam  = Context.team.players[player].leader
        local playerAdded = false
        for _, stage in ipairs(Context.worker.stages) do
            local sameIbTeams = {}
            for _, other in ipairs(stage) do
                local otherIb   = Context.map.ibs[other]
                local otherTeam = Context.team.players[other].leader
                if playerTeam == otherTeam then
                    table.insert(stage, player)
                    playerAdded = true
                    break
                elseif playerIb == otherIb then
                    sameIbTeams[otherTeam] = 0
                end
            end
            if playerAdded then break end
            local sameIbTeamsCount = 0
            for _ in pairs(sameIbTeams) do sameIbTeamsCount = sameIbTeamsCount + 1 end
            if not playerAdded and sameIbTeamsCount < internal.limit.count then
                table.insert(stage, player)
                playerAdded = true
                break
            end
        end
        if not playerAdded then
            table.insert(Context.worker.stages, { player })
        end
    end
end

function worker.evaluate()
    Context.worker        = {}
    Context.worker.stages = {}

    if internal.limit == nil then
        Context.worker.stages[1] = {}
        for i = 1, #Context.map.uuts do
            table.insert(Context.worker.stages[1], i);
        end
        return
    end

    if (internal.limit.target == Team) then
        __evaluate_team()
    elseif (internal.limit.target == Ib) then
        __evaluate_ib()
    else
        error(WorkerError("Invalid Limitation"))
    end
end

function worker.count(count)
    if (count ~= nil) then
        internal.count = count
    end
    return count
end

function internal.limitTo(count, specifier)
    assert(count ~= nil, WorkerError("Count missing"))
    assert(specifier == Team, WorkerError("Invalid specifier: " .. tostring(specifier)))
    internal.limit = { target = internal.scope, count = count, reference = specifier }
end

function worker.limit(specifier)
    assert(internal.limit == nil, WorkerError("Only one limit allowed"))
    if specifier == nil then
        error(WorkerError("Missing specifier"))
    elseif specifier == Ib then
        internal.scope = specifier
    else
        error(WorkerError("Unsupported limit specifier: " .. tostring(specifier)))
    end
    return { To = internal.limitTo }
end

return worker
