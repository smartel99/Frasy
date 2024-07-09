local IniParser = require('lua.core.utils.ini_parser')
local PrettyPrint = require('lua.core.utils.pretty_print')
local ObjectType = require('lua.core.can_open.types.object_type')
local DataType = require('lua.core.can_open.types.data_type')

local function NormalizeParameterName(name) return name end

local function MakeSubIndexKey(index, subIndex)
    return index .. "sub" .. string.format("%X", subIndex)
end

local function IsComplexEntry(field)
    local ot = tonumber(field["ObjectType"])
    return ot == ObjectType.array or ot == ObjectType.record
end

local function ParseNumber(field)
    if type(field) == "string" and #field ~= 0 then
        return tonumber(field)
    else
        return nil
    end
end

local function ParseVarEntry(field)
    local entry = {}
    entry.__kind = "Object Dictionary Entry"
    entry.parameterName = field["ParameterName"]
    entry.objectType = tonumber(field["ObjectType"])
    entry.dataType = tonumber(field["DataType"])
    entry.accessType = field["AccessType"]
    entry.defaultValue = field["DefaultValue"]
    entry.pdoMapping = tonumber(field["PDOMapping"])
    entry.defaultValue = ParseNumber(field["DefaultValue"])
    entry.value = entry.defaultValue
    entry.highLimit = ParseNumber(field["HighLimit"])
    entry.lowLimit = ParseNumber(field["LowLimit"])
    return entry
end

local function ParseComplexEntry(ini, index)
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
        local subKey = MakeSubIndexKey(index, i)
        local subField = ini[subKey]
        local subEntry = ParseVarEntry(subField)
        subEntry.index = "0x" .. index
        subEntry.subIndex = string.format("%x", i)
        if (entry.objectType == ObjectType.array) then
            entry.data[i] = subEntry
        elseif (entry.objectType == ObjectType.record) then
            local name = NormalizeParameterName(subEntry.parameterName)
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

local function ParseObjectDictionary(ini)
    local od = {}
    local indexes = {}
    for k, v in pairs(ini) do
        local index = string.match(k, "^%d+$")
        if (index ~= nil) then table.insert(indexes, index) end
    end

    for _, index in pairs(indexes) do
        local field = ini[index]
        local entry = {}
        if (IsComplexEntry(field)) then
            entry = ParseComplexEntry(ini, index)
            entry.index = "0x" .. index
        else
            entry = ParseVarEntry(field)
            entry.index = "0x" .. index
            entry.subIndex = "0x0"
        end
        local name = NormalizeParameterName(entry.parameterName)
        if (od[name] ~= nil) then error("Duplicate entry: " .. name) end
        od[name] = entry
    end
    return od
end

return {
    Parse = function(str)
        local ini = IniParser.Parse(str)
        return ParseObjectDictionary(ini)
    end,
    LoadFile = function(filename)
        local ini = IniParser.LoadFile(filename)
        return ParseObjectDictionary(ini)
    end
}
