return function(value)
    return
        type(value) == "number" and (value // 1) == value and -128 <= value and
            value <= 127
end
