--- @file    map.lua
--- @author  Paul Thomas
--- @date    3/20/2023
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
local internal = {
    map = {},
    scope = nil,
    count = {
        expected = {tp = 0, ib = 0, uut = 0},
        recount = {tp = 0, ib = 0, uut = 0}
    },
    keys = {tp = {}, ib = {}, uut = {}}
}

if Map == nil then Map = {} end

local mapper = {TestPoint = {}, UUT = {}, IB = {}}

if Context.Map == nil then
    Context.Map = {count = {tp = 0, uut = 0, ib = 0}, uut = {}, ib = {}}
end

function mapper.validate()
    for _ in pairs(internal.keys.tp) do
        internal.count.recount.tp = internal.count.recount.tp + 1
    end
    for _ in pairs(internal.keys.uut) do
        internal.count.recount.uut = internal.count.recount.uut + 1
    end
    for _ in pairs(internal.keys.ib) do
        internal.count.recount.ib = internal.count.recount.ib + 1
    end
    Log.d(string.format("Map has %d test points",
                        internal.count.recount.tp))

    Log.d(string.format("Map has %d uuts:\n%s", internal.count.recount.uut,
                        Utils.tostring(internal.keys.uut)))

    Log.d(string.format("Map has %d instrumentation boards:\n%s",
                        internal.count.recount.ib, Utils.tostring(internal.keys.ib)))

    if internal.count.expected.tp ~= 0 and internal.count.expected.tp ~=
        internal.count.recount.tp then
        error(MapperError("Unexpected TP count. E: " ..
                              internal.count.expected.tp .. ", C: " ..
                              internal.count.recount.tp))
    end
    if internal.count.expected.uut ~= 0 and internal.count.expected.uut ~=
        internal.count.recount.uut then
        error(MapperError("Unexpected UUT count. E: " ..
                              internal.count.expected.uut .. ", C: " ..
                              internal.count.recount.uut))
    end
    if internal.count.expected.ib ~= 0 and internal.count.expected.ib ~=
        internal.count.recount.ib then
        error(MapperError("Unexpected IB count. E: " ..
                              internal.count.expected.ib .. ", C: " ..
                              internal.count.recount.ib))
    end

    for tp, uuts in pairs(internal.map) do
        local count = 0
        for uut, value in pairs(uuts) do
            local iuut = uut
            if internal.count.recount.uut < iuut then
                error(MapperError("Out of range UUT on test point '" .. tp .. "' : " .. iuut))
            end
            -- TODO: Prevents non-sequential IBs from existing, find a way around that
            -- if internal.count.recount.ib < value.ib then
            --     error(MapperError(
            --               "Out of range IB on " .. tp .. " on UUT " .. uut ..
            --                   ". Got " .. value.ib))
            -- end
            count = count + 1
        end
        if count ~= internal.count.recount.uut then
            error(MapperError("Invalid UUT count for " .. tp .. ". E: " ..
                                  internal.count.recount.uut .. ", C: " .. count))
        end
    end

    Context.Map.count.tp = internal.count.recount.tp
    Context.Map.count.uut = internal.count.recount.uut
    Context.Map.count.ib = internal.count.recount.ib

    return internal.map
end

function mapper.TestPoint.New(name)
    internal.scope = name
    internal.keys.tp[name] = 0
end

--- Establishes a connection between a UUT and an instrumentation board.
---@param uut number ID of the UUT.
---@param ib number ID of the Instrumentation board.
---@param tp number ID of the test point on the instrumentation board.
function mapper.TestPoint.To(uut, ib, tp)
    if internal.scope == nil then error(MapperError("No scope provided")) end
    if uut < 1 or
        (internal.count.expected.uut ~= 0 and internal.count.expected.uut < uut) then
        error(MapperError(string.format("Out of range UUT %d", uut)))
    end
    -- TODO: Prevents non-sequential IBs from existing, find a way around that
    -- if ib < 1 or (internal.count.expected.ib ~= 0 and internal.count.expected.ib < ib) then
    --     error(MapperError(string.format("Out of range IB: %d", ib)))
    -- end
    if internal.map[internal.scope] == nil then
        internal.map[internal.scope] = {}
    end
    internal.keys.uut[uut] = 0
    internal.keys.ib[ib] = 0
    internal.map[internal.scope][uut] = {ib = ib, tp = tp}

    if Context.Map.uut[uut] == nil then Context.Map.uut[uut] = {} end
    if Context.Map.uut[uut].ib ~= nil and Context.Map.uut[uut].ib ~= ib then
        error(MapperError(string.format(
                              "Frasy currently support only 1 IB per uut")))
    end
    Context.Map.uut[uut].ib = ib

    if Context.Map.ib[ib] == nil then Context.Map.ib[ib] = {} end
    if Context.Map.ib[ib].uut == nil then Context.Map.ib[ib].uut = {} end
    Context.Map.ib[ib].uut[uut] = uut

    return {To = mapper.TestPoint.To}
end

---Sets the number of expected connexions on this test point
---@param count number The number of expected connexions.
---@return number If count is not provided, returns the expected number of UUTs.
function mapper.TestPoint.Count(count)
    if count ~= nil then internal.count.expected.tp = count end
    return internal.count.expected.tp
end

---Sets the number of expected Units Under Tests (UUT).
---@param count number|nil The number of expected UUTs.
---@return number If count is not provided, returns the expected number of UUTs.
function mapper.UUT.Count(count)
    if count ~= nil then internal.count.expected.uut = count end
    return internal.count.expected.uut
end

---Sets the number of expected Instrumentation board.
---@param count number|nil The number of expected Instrumentation Boards.
---@return number If count is not provided, returns the expected number of instrumentation boards.
function mapper.IB.Count(count)
    if count ~= nil then internal.count.expected.ib = count end
    return internal.count.expected.ib
end

return mapper;
