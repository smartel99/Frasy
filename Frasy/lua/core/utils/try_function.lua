return function(fun, maxTries)
    if type(fun) ~= "function" then error("fun must be function taking no parameter and returning a boolean") end
    if type(maxTries) ~= "number" or maxTries < 1 then error("maxTries must be an integer greater or equal to 1") end

    local tries = 0
    local result = false
    while not result do
        if tries >= maxTries then error("Reached tries limit") end
        tries = tries + 1
        result = fun()
    end
end
