local Ib = {
    kind = 0,
    version = "0.0.0",
    nodeId = 0,
    __type = "ibs"
}
Ib.__index = Ib

function Ib:new(base)
    if (base == nil) then
        return setmetatable({
            kind = 0,
            version = "0.0.0",
            nodeId = 0,
        }, Ib)
    else
        return setmetatable({
            kind = base.kind,
            nodeId = base.nodeId,
            version = base.version,
        }, Ib)
    end
end

function Ib:Kind(kind)
    if (kind == nil) then
        return Ib.kind
    else
--         if (Context.info.stage ~= Stage.Generation) then
--             error("Cannot set Ib kind. Outside of generation stage")
--         end
        if (type(kind) ~= "number") then
            error("Cannot set Ib kind. Type is not a number")
        end
        self.kind = kind
        return self
    end
end

function Ib:Version(requirement)
    if (requirement == nil) then
        return Ib.version
    else
--         if (Context.info.stage ~= Stage.Generation) then
--             error("Cannot set Ib version requirement. Outside of generation stage")
--         end
        self.version = requirement
        return self
    end
end

function Ib:NodeId(id)
    if (id == nil) then
        return self.nodeId
    else
--         if (Context.info.stage ~= Stage.Generation) then
--             error("Cannot set Ib NodeId. Outside of generation stage")
--         end
        if (type(id) ~= "number") then
            error("Cannot set Ib NodeId. ID is not a number")
        end
        if (id < 1 or id > 127) then
            error("Cannot set Ib NodeId. ID is out of scope")
        end
        self.nodeId = id
        return self
    end
end

function Ib:Upload(sdo)
    if (Context.info.stage ~= Stage.Execution) then
        return
    end
    if (type(sdo) == "number") then
        return CanOpen.__upload(self.nodeId, { sdo, nil })
    elseif (type(sdo) == "table") then
        return CanOpen.__upload(self.nodeId, { sdo.index, sdo.subindex })
    else
        error("Ib Upload, invalid sdo")
    end
end

function Ib:Download(sdo, value)
    if (Context.info.stage ~= Stage.Execution) then
        return
    end
    CanOpen.__download(self.nodeId, sdo, value)
end

return Ib
