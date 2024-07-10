return function(value)
    return
        type(value) == "number" and (value // 1) == value and -32768 <= value and
            value <= 32767
end
