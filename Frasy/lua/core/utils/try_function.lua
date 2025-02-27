---@class TryFunctionOptParam
---@field delay_ms integer duration in ms between each tries, default to 0
---@field raiseError boolean will raise an error if reach max tries, default to false

---@param fun function
---@param maxTries integer max number of tries allowed
---@param opt TryFunctionOptParam?
local function tryFunction(fun, maxTries, opt)
    local defaultDelayMs = 10
    local defaultRaiseError = false
    if type(fun) ~= "function" then error("fun must be function taking no parameter and returning a boolean") end
    if type(maxTries) ~= "number" or maxTries < 1 then error("maxTries must be an integer greater or equal to 1") end
    if opt == nil then opt = { delay_ms = defaultDelayMs, raiseError = defaultRaiseError } end
    if type(opt) ~= "table" then error("opt is not a table") end
    if opt.delay_ms == nil then opt.delay_ms = defaultDelayMs end
    if opt.raiseError == nil then opt.raiseError = defaultRaiseError end
    if type(opt.delay_ms) ~= "number" then error("opt.delay_ms is not an integer") end
    if type(opt.raiseError) ~= "boolean" then error("opt.raiseError is not a boolean") end

    local tries = 0
    local result = false
    while not result do
        if tries ~= 0 and opt.delay_ms ~= 0 then SleepFor(opt.delay_ms) end
        if tries >= maxTries then
            if opt.raiseError then error("Reached tries limit") end
            return
        end
        tries = tries + 1
        result = fun()
    end
end

return tryFunction
