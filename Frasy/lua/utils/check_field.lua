---@param value any The value to check
---@param name string A name to be shown in the error message
---@param predicate function A predicate that validates the value
return function(value, name, predicate)
    assert(predicate, "Invalid " .. tostring(name) .. ": " .. tostring(value))
end
