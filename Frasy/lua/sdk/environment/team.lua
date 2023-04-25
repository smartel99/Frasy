--- @file    team.lua
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

Team = {}
if Context.Team == nil then
    Context.Team = {
        players = {},
        teams   = {},
        hasTeam = false,
    }
end

Team.Status = {
    pass             = 0,
    fail             = 1,
    critical_failure = 2,
}

--- Create a new team
--- @vararg number the UUT to join the team. Provided in order, first is leader
function Team.Join(...)
    Context.Team.hasTeam = true
    local leader
    for index, uut in ipairs({ ... }) do
        if index == 1 then leader = uut end
        if Context.Team.players[uut] ~= nil then
            TeamError(string.format("Player %d is already in team %d", uut, leader))
        end
        Context.Team.players[uut] = { leader = leader, position = index }
    end
    Context.Team.teams[leader] = { ... }
end

function Team.GetLeader()
    return Context.Team.players[Context.uut].leader
end

function Team.IsLeader()
    return Context.Team.players[Context.uut].position == 1
end

function Team.Position()
    return Context.Team.players[Context.uut].position
end

function Team.Tell(value)
    Team.__tell(value)
end

function Team.Get()
    local result = Team.__get()
    if result == nil then error(TeamError("No result from get")) end
    return result
end

function Team.Sync(result)
    if (Team.HasTeam()) then
        local status
        if result.stop == 0 then
            status = Team.Status.critical_failure
        elseif result.skipped or not result.pass then
            status = Team.Status.fail
        else
            status = Team.Status.pass
        end
        team_status = Team.__sync(status)
        if team_status ~= status then
            if team_status == Team.Status.fail then
                result.pass   = false
                result.reason = "Teammate failure"
            elseif team_status == Team.Status.critical_failure then
                result.pass   = false
                result.reason = "Teammate critical failure"
                error(TeamError("Teammate critical failure"))
            else
                error(TeamError("Invalid team status"))
            end
        end
    end
end

function Team.Fail()
    if (Team.HasTeam()) then
        Team.__fail()
    end
end

function Team.HasTeam()
    return Context.Team.hasTeam
end

-- C++ calls
function Team.__tell(value) end
function Team.__get() end
function Team.__sync(status) end
function Team.__fail() end

-- not exposed to user
local team = {}
function team.validate()
    if not Context.Team.hasTeam then
        Log.d("Team Not enabled")
        return
    end

    local playersCount = 0
    for _, _ in pairs(Context.Team.players) do
        playersCount = playersCount + 1
    end
    if playersCount ~= Context.Map.count.uut then
        error(TeamError(string.format(
                "Expected %d players, got %d",
                Context.Map.count.uut, playersCount)))
    end

    for leader, players in ipairs(Context.Team.teams) do
        local teamIb = Context.Map.uut[leader].ib
        for uut, info in pairs(players) do
            local playerIb = Context.Map.uut[uut].ib
            if playerIb ~= teamIb then
                error(TeamError(string.format(
                        "IB missmatch. Team %d IB is %d, Player %d IB is %d",
                        leader, teamIb, uut, playerIb)))
            end
        end
    end

    Log.d("Team OK!")
end
return team
