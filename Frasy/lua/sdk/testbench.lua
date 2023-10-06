--- @file    testbench.lua
--- @author  Paul Thomas
--- @date    3/15/2023
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
--- @class Testbench
--- @field private commands table List of all the commands supported by the instrumentation board.
--- @field private ibUsed table List of the instrumentation boards that were accessed during the test.
if Context.Testbench == nil then Context.Testbench =
    {commands = {}, ibUsed = {}} end

--- Request the testbench to execute a command
--- Allows testbench to know on which board to run a command
--- and to get the proper test point values to send
--- @param command string the name of the command to run
--- @param executor function
--- @param ... number Test points to resolve
--- @return any forward the result of the requested command
--- Example usage:
---     Testbench("MyCommand", function(cmd, ...) return cmd(..., "other args") end, Map.TP1, Map.TP2)
function Testbench(command, executor, ...)
    local args = {...}
    local tps = {}
    local ib = 0
    local uut = Context.uut

    -- Get the test points for the current UUT.
    for _, tp in ipairs(args) do
        -- TODO: make an error message pop if the test point does not exist.
        if tp[uut] == nil then
            error(string.format(
                      "UUT %d does not have the #%d provided test point", uut, _))
        end
        table.insert(tps, tp[uut].tp)
        if ib ~= 0 and ib ~= tp[uut].ib then
            error("Requesting TP on different IB")
        end
        ib = tp[uut].ib
    end

    if command == nil then error("A command needs to be provided!") end
    if executor == nil then
        error("An executor function needs to be provided!")
    end
    if ib == 0 then error("No board selected") end
    local cmd = Context.Testbench.commands[command]
    if cmd == nil then error(string.format("Unknown command: %s", command)) end

    -- Remember than we've used this instrumentation board for the logs.
    if Context.Testbench.ibUsed[ib] == nil then
        Context.Testbench.ibUsed[ib] = true
    end

    return executor(function(...)
        local args = {...}
        local result = cmd(ib, table.unpack(args))
        if type(result) == "table" then
            return table.unpack(result)
        else
            return result
        end
    end, table.unpack(tps))
end
