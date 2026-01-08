local IsArray = require("lua/core/utils/is_array/is_array")

--- Checks if a value is an array with a length under the maximum length specified in the object dictionary.
--- @param v any
---@param od OdEntry
return function(v, od)
    return IsArray(v, od.stringLengthMin)
end
