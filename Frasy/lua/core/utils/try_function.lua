return function(fun, maxTries, delay_ms)
    if type(fun) ~= "function" then error("fun must be function taking no parameter and returning a boolean") end
    if type(maxTries) ~= "number" or maxTries < 1 then error("maxTries must be an integer greater or equal to 1") end
    if type(delay_ms) ~= "number" then delay_ms = 10 end
    if delay_ms < 0 then delay_ms = 10 end

    local tries = 0
    local result = false
    while not result do
        if tries ~= 0 then Utils.SleepFor(delay_ms) end
        if tries >= maxTries then error("Reached tries limit") end
        tries = tries + 1
        result = fun()
    end
end
