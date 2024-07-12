return function(value)
    return type(value) == "number" and (value // 1) == value and -2147483648 <= value and value <= 2147483647
end
