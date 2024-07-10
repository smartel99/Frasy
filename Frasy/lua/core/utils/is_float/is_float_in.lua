return function(value, min, max)
    return type(value) == "number" and min <= value and value <= max
end
