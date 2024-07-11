-- --- @file    export_values.lua
-- --- @author  Samuel Martel
-- --- @date    2023/05/31
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

-- Sequence("Test", function()
--     Test("Power Up", function()
--         Requires(Test():ToBeFirst())
--         Expect(12):ToBeInRange(11, 13):ExportAs("voltage")
--     end)

--     Test("U7", function()
--         Requires(Test("Power Up"):ToPass()) -- This is important, to ensure that the value actually exists!
--         local voltage = Test("Power Up"):Value("voltage")
--         Log.D(string.format("Voltage from Power Up: %d", voltage))
--     end)
-- end)

-- -- You can't do that!
-- -- Sequence("Test2", function()
-- --     Requires(Sequence("Test"):ToPass())

-- --     Test("Test", function()
-- --         local voltage = Sequence("Test"):Test("Power Up"):Value("voltage")
-- --         Log.D(string.format("Voltage from Test:Power Up: %d", voltage))
-- --     end)
-- -- end)