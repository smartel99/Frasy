return function(value, od)
    return type(value) == "number" and (value // 1) == value and od.lowLimit <=
               value and value <= od.highLimit
end
