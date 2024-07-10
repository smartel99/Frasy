return function(value, od)
    Utils.Print(od)
    return type(value) == "number" and od.lowLimit <= value and value <= od.highLimit
end
