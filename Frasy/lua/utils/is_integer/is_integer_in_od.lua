return function(value, od)
    return type(value) == "number" and (value // 1) == value and od.LowLimit <=
               value and value <= od.HighLimit
end
