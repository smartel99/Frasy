return function(value, od)
    return type(value) == "number" and od.LowLimit <= value and value <= od.HighLimit
end
