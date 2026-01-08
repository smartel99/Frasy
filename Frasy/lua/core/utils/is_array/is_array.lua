--- Check if a value is an array, optionally validating its length.
--- @param v any The value to check
--- @param len number|nil The max length of the array.
return function(v, len)
    if len ~= nil then
        return type(v) == "table" and #v <= len
    else
        return type(v) == "table"
    end
end
