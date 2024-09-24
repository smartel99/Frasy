local IsInteger = require("lua/core/utils/is_integer/is_integer")
--- Validates that a number is within the bounds dictated by the enum
--- @param value integer
--- @param enum table
return function(value, enum)
    if type(enum) ~= "table" then error("enum parameter is not a table") end
    if enum.__enum == nil then
        -- Populate cached enum informations
        local count = 0
        local min
        local max
        for _, v in pairs(enum) do
            if not IsInteger(v) then error("enum is not an enum table") end
            if min == nil or v < min then min = v end
            if max == nil or v > max then max = v end
            count = count + 1
        end
        enum.__enum = { count = count, min = min, max = max }
    end
    return type(value) == "number" and (value // 1) == value and enum.__enum.min <= value and value <= enum.__enum.max
end
