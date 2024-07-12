return function(value, min, max)
    return type(value) == "number" and (value // 1) == value and min < value and value < max
end
