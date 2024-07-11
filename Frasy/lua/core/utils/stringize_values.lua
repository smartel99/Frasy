---Packs a list of integral values into a string using the 'B' flag for string.pack.
---@vararg integer
---@return string string
return function(...)
    local len = select("#", ...)
    local fmt = string.rep("B", len)
    return string.pack(fmt, ...)
end
