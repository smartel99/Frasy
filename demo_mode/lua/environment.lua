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
    Team.Join(1, 2, 3, 4) -- Makes a team with UUT 1 as the leader, and UUT 2, 3 and 4 as members.
    Team.Join(5, 6, 7, 8)
    Team.Join(9, 10)

    -- Limits the number of teams that can be simultaneously be executed on every instrumentation board.
    Worker.Limit(IB).To(1, Team)

    TestPoint("TP1") -- Creates a test point called TP1
            .To(01, 1, 01)  -- Connects that test point between UUT 1 and TP1 of instrumentation card 1.
            .To(02, 1, 02)  -- Connects that test point between UUT 1 and TP2 of instrumentation card 1.
            .To(03, 1, 03)  -- Connects that test point between UUT 1 and TP3 of instrumentation card 1.
            .To(04, 1, 04)  -- Connects that test point between UUT 1 and TP4 of instrumentation card 1.
            .To(05, 1, 05)  -- Connects that test point between UUT 1 and TP5 of instrumentation card 1.
            .To(06, 1, 06)  -- Connects that test point between UUT 1 and TP6 of instrumentation card 1.
            .To(07, 1, 07)  -- Connects that test point between UUT 1 and TP7 of instrumentation card 1.
            .To(08, 1, 08)  -- Connects that test point between UUT 1 and TP8 of instrumentation card 1.
            .To(09, 1, 09)  -- Connects that test point between UUT 1 and TP9 of instrumentation card 1.
            .To(10, 1, 10)  -- Connects that test point between UUT 1 and TP10 of instrumentation card 1.
end)
