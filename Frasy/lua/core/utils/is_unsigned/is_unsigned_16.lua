return function(value)
    return type(value) == "number" and (value // 1) == value and 0 <= value and
               value <= 65535
end
