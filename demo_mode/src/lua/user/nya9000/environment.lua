--- @file    environment.lua
--- @author  Paul Thomas
--- @date    4/3/2023
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

Environment.Make(function()
    -- Tell how many UUT we have
    Environment.Uut.Count(1)

--     Team.Add(1, 2, 3, 4) -- {L: 1, M: {2, 3, 4}}. Create team with UUT 1 as the leader and UUT 2, 3 and 4 as members.
--     Team.Add(5, 6, 7, 8) -- {L: 5, M: {6, 7, 8}}
--     Team.Add(9, 10)      -- {L: 9, M: {10}}

    -- -- Limits the number of teams that can be simultaneously executed
    -- Worker.Limit(Team).To(1)

    -- Define IBs
    local daq = Environment.Ib.Add(DAQ:New())
    --local pio = Environment.Ib.Add(PIO:New())
    --local r8l = Environment.Ib.Add(R8L:New(nil, 26))
end)
