--- @class Ib
--- @field kind integer
--- @field nodeId integer
--- @field eds string
--- @field name string
--- @field od table
--- @field private __kind string
local Ib = { kind = 0, nodeId = 0, eds = "", name = "", __kind = "ib", }
Ib.__index = Ib

--- Creates a new instrumentation board.
--- @return Ib
function Ib:New()
    return setmetatable({ kind = 0, nodeId = 0, eds = "", name = "" }, Ib)
end

--- Upload (fetches) an entry from the node.
--- @param ode OdEntry
--- @return OdEntryType
function Ib:Upload(ode)
    assert(type(ode) == "table", "Ib upload, invalid ode")
    assert(ode.__kind == "Object Dictionary Entry",
        "Ib upload, argument is not an Object Dictionary Entry. " ..
        ode.__kind)
    if (ode.objectType == CanOpen.objectType.var) then
        if (Context.info.stage ~= Stage.execution) then return ode.value end
        ode.value = CanOpen.__upload(self.nodeId, ode)
    elseif (ode.objectType == CanOpen.objectType.array) then
        -- do we need to update ode actual size here?
        -- I feel like this should be done by the user who's
        -- manipulating the entry
        -- FIXME wtf is going on?
        for i = 1, ode.data[0].value do self:Upload(v) end
    elseif (ode.objectType == CanOpen.objectType.record) then
        for k, v in ipairs(ode.__fields) do self:Upload(ode[v]) end
    else
        error("Ib upload, invalid object type")
    end
    return ode.value
end

--- Download (sends) an entry to the node.
--- @param ode OdEntry
--- @param value OdEntryType|OdEntryArrayType
function Ib:Download(ode, value)
    if (Context.info.stage ~= Stage.execution) then return end
    assert(type(ode) == "table", "Ib download, invalid ode")
    assert(ode.__kind == "Object Dictionary Entry",
        "Ib download, not an Object Dictionary Entry")
    if (ode.objectType == CanOpen.objectType.var) then
        CanOpen.__download(self.nodeId, ode, value)
    elseif (ode.objectType == CanOpen.objectType.array) then
        for k, v in ipairs(value --[[@as OdEntryArrayType]]) do Ib:Download(ode.data[k], v) end
    elseif (ode.objectType == CanOpen.objectType.record) then
        for k, v in pairs(ode.__fields) do
            Ib:Download(ode[v], value[v])
        end
    else
        error("Ib download, invalid object type")
    end
end

function Ib:Reset()
    CanOpen.__reset(self.nodeId)
end

function Ib:Serial()
    return self:Upload(self.od["Identity"]["Serial number"])
end

function Ib:SoftwareVersion()
    return self:Upload(self.od["Manufacturer software version"])
end

function Ib:HardwareVersion()
    return self:Upload(self.od["Manufacturer hardware version"])
end

return Ib
