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
if Context.team == nil then
    Context.team = {
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
function Team.Wait(fun)
    assert(Team.IsLeader(), TeamError("Only leader can wait for others"))
    assert(type(fun) == "function", TeamError("Invalid routine provided: " .. type(fun)))
    Team.__wait(fun)
end

function Team.Done()
    assert(not Team.IsLeader, TeamError("Only teammate can report as done"))
    Team.__done()
end

function Team.GetLeader()
    return Context.team.players[Context.info.uut].leader
end

function Team.IsLeader()
    return Context.team.players[Context.info.uut].position == 1
end

function Team.Position()
    return Context.team.players[Context.info.uut].position
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
        if result.time.stop == 0 then
            status = Team.Status.critical_failure
        elseif result.skipped or not result.pass then
            status = Team.Status.fail
        else
            status = Team.Status.pass
        end
        local team_status = Team.__sync(status)
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
    return Context.team.hasTeam
end

-- C++ calls
function Team.__tell(value) end

function Team.__get() end

function Team.__wait(fun) end

function Team.__done() end

function Team.__sync(status) end

function Team.__fail() end

-- not exposed to user
local team = {}
function team.validate()
    if not Context.team.hasTeam then
        Log.d("Team Not enabled")
        return
    end

    local playersCount = 0
    for _, _ in pairs(Context.team.players) do
        playersCount = playersCount + 1
    end
    assert(playersCount == #Context.map.uuts,
        TeamError(string.format("Expected %d players, got %d", #Context.map.uuts, playersCount)))
    Log.d("Team OK!")
end

return team
