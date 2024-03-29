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

function worker.evaluate()
    Context.Worker        = { }
    Context.Worker.stages = {}

    if internal.limit == nil then
        Context.Worker.stages[1] = {}
        for i = 1, Context.Map.count.uut do
            table.insert(Context.Worker.stages[1], i);
        end
        return
    end

    if internal.limit.target ~= IB then
        error(WorkerError("Invalid target specifier"))
    end

    if internal.limit.reference ~= Team then
        error(WorkerError("IB limitation must be linked to Team"))
    end

    for player in pairs(Context.Team.players) do
        local playerIb    = Context.Map.ib[player]
        local playerTeam  = Context.Team.players[player].leader
        local playerAdded = false
        for _, stage in ipairs(Context.Worker.stages) do
            local sameIbTeams = {}
            for _, other in ipairs(stage) do
                local otherIb   = Context.Map.ib[other]
                local otherTeam = Context.Team.players[other].leader
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
            table.insert(Context.Worker.stages, { player })
        end
    end
end

function worker.count(count)
    if (count ~= nil) then
        internal.count = count
    end
    return count
end

function internal.limitTo(count, specifier)
    if count == nil then
        error(WorkerError("Count missing"))
    end
    if specifier ~= Team then error(WorkerError("Invalid specifier: " .. tostring(specifier)))
    end
    internal.limit = { target = internal.scope, count = count, reference = specifier }
end

function worker.limit(specifier)
    if internal.limit ~= nil then
        error(WorkerError("Only one limit allowed"))
    end
    if specifier == nil then
        error(WorkerError("Missing specifier"))
    elseif specifier == IB then internal.scope = specifier
    else
        error(WorkerError("Unsupported limit specifier: " .. tostring(specifier)))
    end
    return { To = internal.limitTo }
end

return worker