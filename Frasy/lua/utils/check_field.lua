return function(value, name, predicate)
    assert(predicate, "Invalid " .. tostring(name) .. ": " .. tostring(value))
end
