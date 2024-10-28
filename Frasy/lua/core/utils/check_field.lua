--- @param value any The value to check
--- @param name string A name to be shown in the error message
--- @param isValid boolean Result of a user check against value
return function(value, name, isValid)
    assert(isValid, "Invalid " .. tostring(name) .. ": " .. ToString(value))
end
