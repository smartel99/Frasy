--- Validates that a number is within the bounds dictated by the object dictionary entry.
--- @param value integer
--- @param od OdEntry
return function(value, od)
    if od.lowLimit == nil then error(string.format("OD entry '%s' does not have a lower limit", od.parameterName)) end
    if od.highLimit == nil then error(string.format("OD entry '%s' does not have a higher limit", od.parameterName)) end
    return type(value) == "number" and (value // 1) == value and od.lowLimit <= value and value <= od.highLimit
end
