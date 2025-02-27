--- Checks whether a value is in a table or not.
---@param t array The table to check
---@param v any The value to find
return function(t, v)
    for _, e in ipairs(t) do
        if type(e) == type(v) and e == v then return true end
    end
    return false
end
