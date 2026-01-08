--- @file    global.lua
--- @author  Paul Thomas
--- @date    2023-03-15
--- @brief   Collection of globally declared utility calls
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

if Print ~= nil then error("Common utils already defined. Do not required this file") end

--- Convert a lua object to a string
--- Unlike tostring, it will print a table content
--- @param t any Object to convert to string
--- @param max_depth number? max depth when converting a table to string, default to infinite depth
--- @param depth number? Current level, user should not provide that argument
--- @return string
local function PrettyPrint(t, max_depth, depth)
    if (type(t) == "table") then
        local indent = ""
        if max_depth == nil then max_depth = -1 end
        if depth == nil then depth = 0 end
        for _ = 1, depth do indent = indent .. "  " end
        local str = ""
        local first = true
        for k, v in pairs(t) do
            if (not first or depth ~= 0) then str = str .. "\n" end
            if (first) then first = false end
            if (type(max_depth) == "number" and max_depth == depth) then
                str = str .. indent .. tostring(k) .. ": " .. tostring(v)
            else
                str = str .. indent .. tostring(k) .. ": " ..
                    PrettyPrint(v, max_depth, depth + 1)
            end
        end
        return str
    elseif (type(t) == "function" or type(t) == "thread" or type(t) ==
            "userdata") then
        return type(t)
    else
        return tostring(t)
    end
end

--- Print a Lua Object
--- Unlike lua print(), it will print tables content
--- @param t any Object to print
--- @param max_depth number? max depth to print if passing a table, default to infinite depth
function Print(t, max_depth)
    if (type(t) == "table") then
        print(PrettyPrint(t, max_depth))
    else
        print(t)
    end
end

--- Convert a Lua Object to a String
--- Unlike lua tostring(), it will convert tables content
--- @param t any Object to print
--- @param max_depth number? max depth to convert if passing a table, default to infinite depth
--- @return string
function ToString(t, max_depth)
    if (type(t) == "table") then
        local maybeMeta = getmetatable(t)
        if maybeMeta ~= nil and type(maybeMeta.__tostring) == "function" then
            return tostring(t)
        end
        return PrettyPrint(t, max_depth)
    else
        return tostring(t)
    end
end

--- Check if two object are equals
--- Unlike lua basic == operator, it also check tables content.
--- Only check contents, not object pointers
--- @param t1 any Object to compare
--- @param t2 any Object to compare
--- @return boolean
function Equals(t1, t2)
    if type(t1) ~= type(t2) then
        return false
    end
    if type(t1) == "table" then
        -- Check both table have the same number of keys
        -- If not, then tables cannot be identical
        local t1_keys = 0
        local t2_keys = 0
        for k in pairs(t1) do
            t1_keys = t1_keys + 1
        end
        for k in pairs(t2) do
            t2_keys = t2_keys + 1
        end
        if t1_keys ~= t2_keys then
            return false
        end

        -- Check value of both table
        -- It also check that they share the same key since if they do not, one value would be nil
        for k, _ in pairs(t1) do
            if not Equals(t1[k], t2[k]) then
                return false
            end
        end
        return true
    else
        return t1 == t2
    end
end

--- Get value from a table by passing chain of keys
--- If the chain of keys is broken, will return nil
--- ie. for table A = {B = {C = 0}}
--- Traverse(A, "B", "C") == 0, Chain of keys OK
--- Traverse(A, "B", "D") == nil, Chain of keys broken at end
--- Traverse(A, "C", "B") == nil, Chain of keys broken in the middle
--- @param t table Table to access
--- @param ... string chain of keys
--- @return any value
function Traverse(t, ...)
    local args = { ... }
    if #args == 0 or t == nil or t[args[1]] == nil then
        return t
    else
        local st = t[args[1]]
        table.remove(args, 1)
        return Traverse(st, table.unpack(args))
    end
end

--- Split a string per line
--- ie. "Hello\nWorld" => ['Hello', 'World']
--- @param content string text to split
--- @return table lines
function LineSplit(content)
    local lines = {}
    for line in string.gmatch(content, "^[\r\n]+") do
        table.insert(lines, line)
    end
    return lines
end

--- Convert a number to an integer
--- @param value number
--- @return integer
function ToInt(value)
    if (value >= 0) then return math.floor(value + 0.5) else return math.ceil(value + 0.5) end
end

--- Get List of files and folder from a directory
--- @param path string folder path where to look
--- @return table entries
function DirList(path) return {} end -- C++ call

--- Pause the program for set duration
--- @param ms integer duration in ms
function SleepFor(ms) end -- C++ call

--- Combines 4 bytes in big-endian and bit-casts them into a float
--- @param data integer[] Array of 4 bytes
--- @return number value The float represented by the 4 bytes.
function CombineAndBitcast(data) return 0 end -- C++ call

--- Save a table to a JSON file
--- @param table table Table to save
--- @param filepath string filepath where to save the json
function SaveAsJson(table, filepath) end -- C++ call
