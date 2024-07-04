local Ib = {
    kind = 0,
    nodeId = 0,
    eds = "",
    name = "",
    __kind = "ib",
}
Ib.__index = Ib

function Ib:new()
    return setmetatable({
        kind = 0,
        nodeId = 0,
        eds = "",
        name = name
    }, Ib)
end

function Ib:Upload(ode)
    assert(type(ode) == "table", "Ib upload, invalid ode")
    assert(ode.__kind == "Object Dictionary Entry",
           "Ib upload, argument is not an Object Dictionary Entry. " ..
               ode.__kind)
    Utils.print("Node id: " .. tostring(self.nodeId))
    if (ode.objectType == CanOpen.objectType.var) then
        if (Context.info.stage ~= Stage.Execution) then return ode.value end
        ode.value = CanOpen.__upload(self.nodeId, ode)
    elseif (ode.objectType == CanOpen.objectType.array) then
        -- do we need to update ode actual size here?
        -- I feel like this should be done by the user who's
        -- manipulating the entry
        for i = 1, ode.data[0].value do self:Upload(v) end
    elseif (ode.objectType == CanOpen.objectType.record) then
        for k, v in ipairs(ode.__fields) do self:Upload(ode[v]) end
    else
        error("Ib upload, invalid object type")
    end
    return ode.value
end

function Ib:Download(ode, value)
    if (Context.info.stage ~= Stage.Execution) then return end
    assert(type(ode) == "table", "Ib download, invalid ode")
    assert(ode.__kind == "Object Dictionary Entry",
           "Ib download, not an Object Dictionary Entry")
    if (ode.objectType == CanOpen.objectType.var) then
        CanOpen.__download(self.nodeId, ode, value)
    elseif (ode.objectType == CanOpen.objectType.array) then
        for k, v in ipairs(value) do Ib:Download(ode.data[k], v) end
    elseif (ode.objectType == CanOpen.objectType.record) then
        for k, v in pairs(ode.__fields) do Ib:Download(ode[v], value[v]) end
    else
        error("Ib download, invalid object type")
    end
end

return Ib
