--- @file    is.lua
--- @author  Paul Thomas
--- @date    2026-06-03
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

local Is = {}

local function isOdIn(od) return Is.Table(od) and Is.Not(od.lowLimit, Is.Nil) and Is.Not(od.highLimit, Is.Nil) end

function Is.Array(v) return Is.Table(v) and #v ~= 0 end

function Is.ArrayInOd(v, od) return Is.Array(v) and #v <= od.stringLengthMin end

function Is.InArray(v, t)
    if not Is.Array(v) then return false end
    for _, e in ipairs(t) do
        if type(e) == type(v) and e == v then return true end
    end
    return false
end

function Is.Boolean(v) return type(v) == "boolean" end

function Is.Float(v) return type(v) == "number" end

function Is.FloatIn(v, min, max) return Is.Float(v) and min <= v and v <= max end

function Is.FloatInEx(v, min, max) return Is.Float(v) and min < v and v < max end

function Is.FloatInOd(v, od) return isOdIn(od) and Is.FloatIn(v, od.lowLimit, od.highLimit) end

function Is.Function(v) return type(v) == "function" end

function Is.Integer(v) return Is.Number(v) and (v // 1) == v end

function Is.Integer8(v) return Is.IntegerIn(v, -128, 127) end

function Is.Integer16(v) return Is.IntegerIn(v, -32768, 32767) end

function Is.Integer32(v) return Is.IntegerIn(v, -2147483648, 2147483647) end

function Is.IntegerIn(v, min, max) return Is.Integer(v) and min <= v and v <= max end

function Is.IntegerInEx(v, min, max) return Is.Integer(v) and min < v and v < max end

function Is.IntegerInOd(v, od) return isOdIn(od) and Is.IntegerIn(v, od.lowLimit, od.highLimit) end

function Is.Nil(v) return v == nil end

function Is.Not(v, f, ...) return not f(v, ...) end

function Is.Number(v) return type(v) == "number" end

function Is.String(v) return type(v) == "string" end

function Is.Table(v) return type(v) == "table" end

function Is.Unsigned(v) return Is.Integer(v) and 0 <= v end

function Is.Unsigned8(v) return Is.UnsignedIn(v, 0, 255) end

function Is.Unsigned16(v) return Is.UnsignedIn(v, 0, 65535) end

function Is.Unsigned32(v) return Is.UnsignedIn(v, 0, 4294967295) end

function Is.UnsignedIn(v, min, max) return Is.Unsigned(v) and min <= v and v <= max end

function Is.UnsignedInEx(v, min, max) return Is.Unsigned(v) and min < v and v < max end

function Is.UnsignedInOd(v, od) return isOdIn(od) and Is.UnsignedIn(v, od.lowLimit, od.highLimit) end

return Is
