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

Environment(function()
    -- Tell how many UUT we have
    Uut.Count(1)

--     Team.Add(1, 2, 3, 4) -- {L: 1, M: {2, 3, 4}}. Create team with UUT 1 as the leader and UUT 2, 3 and 4 as members.
--     Team.Add(5, 6, 7, 8) -- {L: 5, M: {6, 7, 8}}
--     Team.Add(9, 10)      -- {L: 9, M: {10}}

    -- -- Limits the number of teams that can be simultaneously executed
    -- Worker.Limit(Team).To(1)

    -- Define IBs
    --local daq = Ib.Add(Ibs.daq)
    local pio = Ib.Add(Ibs.pio):NodeId(2)
    local r8l = Ib.Add(Ibs.r8l):NodeId(26)


    -- UUT Values
    -- Will be tree-shaken per uuts
    UutValue.Add("tpPower")
        .Link(01, TestPoint(daq, 01))
--         .Link(02, TestPoint(daq, 02))
--         .Link(03, TestPoint(daq, 03))
--         .Link(04, TestPoint(daq, 04))
--         .Link(05, TestPoint(daq, 05))
--         .Link(06, TestPoint(daq, 06))
--         .Link(07, TestPoint(daq, 07))
--         .Link(08, TestPoint(daq, 08))
--         .Link(09, TestPoint(daq, 09))
--         .Link(10, TestPoint(daq, 10))

    UutValue.Add("tpRelay")
        .Link(01, TestPoint(pio, 01))
--         .Link(02, TestPoint(pio, 01))
--         .Link(03, TestPoint(pio, 01))
--         .Link(04, TestPoint(pio, 01))
--         .Link(05, TestPoint(pio, 02))
--         .Link(06, TestPoint(pio, 02))
--         .Link(07, TestPoint(pio, 02))
--         .Link(08, TestPoint(pio, 02))
--         .Link(09, TestPoint(pio, 03))
--         .Link(10, TestPoint(pio, 03))
end)
