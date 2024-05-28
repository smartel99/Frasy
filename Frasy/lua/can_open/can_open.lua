if (CanOpen ~= nil) then return end

CanOpen = {}

CanOpen.dataType = require("lua.core.can_open.types.data_type")
CanOpen.objectType = require("lua.core.can_open.types.object_type")
CanOpen.objectDictionnary = require("lua.core.can_open.object_dictionnary")

-- C++ functions
-- see orchestrator.cpp
CanOpen.__upload = function(nodeId, ode) error("Not loaded") end
CanOpen.__download = function(nodeId, ode, value) error("Not loaded") end
