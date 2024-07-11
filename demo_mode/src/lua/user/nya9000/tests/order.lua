-- --- @file    s1.lua
-- --- @author  Paul Thomas
-- --- @date    4/3/2023
-- --- @brief
-- ---
-- --- @copyright
-- --- This program is free software: you can redistribute it and/or modify it under the
-- --- terms of the GNU General Public License as published by the Free Software Foundation, either
-- --- version 3 of the License, or (at your option) any later version.
-- --- This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
-- --- even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
-- --- General Public License for more details.
-- --- You should have received a copy of the GNU General Public License along with this program. If
-- --- not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.

-- Sequence("Order", function()
--     Requires(Sequence():ToBeFirst())
--     Test("T1", function()
--         Requires(Test():ToBeFirst())
--     end)

--     Test("T2", function()
--         Requires(Test("T1"):ToBeBefore())
--         Requires(Test("T3"):ToBeAfter())
--     end)

--     Test("T3", function()  end)

--     Test("T4", function()
--         Requires(Test():ToBeLast())
--     end)
-- end)