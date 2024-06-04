local iniParser = require('lua.core.utils.ini_parser')
local pprint = require('lua.core.utils.print')
local ObjectType = require('lua.core.can_open.types.object_type')
local DataType = require('lua.core.can_open.types.data_type')

local function extractIndex(entry) return string.match(entry, "0x(%d+)") end

local function normalizeParameterName(name) return name end

local function makeSubIndexKey(index, subIndex)
    return index .. "sub" .. string.format("%X", subIndex)
end

local function isComplexEntry(field)
    local ot = tonumber(field["ObjectType"])
    return ot == ObjectType.array or ot == ObjectType.record
end

local function parseVarEntry(field)
    local entry = {}
    entry.__kind = "Object Dictionary Entry"
    entry.parameterName = field["ParameterName"]
    entry.objectType = tonumber(field["ObjectType"])
    entry.dataType = tonumber(field["DataType"])
    entry.accessType = field["AccessType"]
    entry.defaultValue = field["DefaultValue"]
    entry.pdoMapping = tonumber(field["PDOMapping"])
    entry.defaultValue = field["DefaultValue"]
    entry.value = entry.defaultValue
    entry.highLimit = field["HighLimit"]
    entry.lowLimit = field["LowLimit"]
    return entry
end

local function parseComplexEntry(ini, index)
    local field = ini[index]
    local entry = {}
    entry.__kind = "Object Dictionary Entry"
    entry.parameterName = field["ParameterName"]
    entry.objectType = tonumber(field["ObjectType"])
    entry.subNumber = tonumber(field["SubNumber"])
    if (entry.objectType == ObjectType.array) then
        entry.data = {}
    elseif (entry.objectType == ObjectType.record) then
        entry.__fields = {} -- used for upload/download
    end
    for i = 0, entry.subNumber - 1 do
        local subKey = makeSubIndexKey(index, i)
        local subField = ini[subKey]
        local subEntry = parseVarEntry(subField)
        subEntry.index = "0x" .. index
        subEntry.subIndex = string.format("%x", i)
        if (entry.objectType == ObjectType.array) then
            entry.data[i] = subEntry
        elseif (entry.objectType == ObjectType.record) then
            local name = normalizeParameterName(subEntry.parameterName)
            if (entry[name] ~= nil) then
                error("Duplicate subentry. " .. name)
            end
            entry[name] = subEntry
            if (i ~= 0) then entry.__fields[i] = name end
        else
            error("Invalid complex type")
        end
    end
    return entry
end

local function parseObjectDictionary(ini)
    local od = {}
    local indexes = {}
    for k, v in pairs(ini) do
        local index = string.match(k, "^%d+$")
        if (index ~= nil) then table.insert(indexes, index) end
    end

    for _, index in pairs(indexes) do
        local field = ini[index]
        local entry = {}
        if (isComplexEntry(field)) then
            entry = parseComplexEntry(ini, index)
            entry.index = "0x" .. index
        else
            entry = parseVarEntry(field)
            entry.index = "0x" .. index
            entry.subIndex = "0x0"
        end
        local name = normalizeParameterName(entry.parameterName)
        if (od[name] ~= nil) then error("Duplicate entry: " .. name) end
        od[name] = entry
    end
    return od
end

return {
    parse = function(str)
        local ini = iniParser.parse(str)
        return parseObjectDictionary(ini)
    end,
    loadFile = function(filename)
        local ini = iniParser.loadFile(filename)
        return parseObjectDictionary(ini)
    end
}
