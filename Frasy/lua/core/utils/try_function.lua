local Is = require("lua/core/utils/is")
local CheckField = require("lua/core/utils/check_field")

---@class TryFunctionOptParam
---@field maxTryCount integer? default to 3
---@field delay integer? delay between each tries in ms, default to 0
---@field raiseError boolean? will raise an error if reach max tries, default to false

---@param fun fun(integer?): boolean
---@param opt TryFunctionOptParam?
---@return boolean, any?
local function tryFunction(fun, opt)
    CheckField(fun, "fun", Is.Function(fun))
    opt = opt or {}
    CheckField(opt, "opt", Is.Table(opt))
    local maxTryCount = opt.maxTryCount or 3
    local delay = opt.delay or 10
    local raiseError = opt.raiseError or false
    Print(type(raiseError))
    CheckField(maxTryCount, "opt.maxTryCount", Is.Unsigned(maxTryCount))
    CheckField(delay, "opt.delay", Is.Unsigned(delay))
    CheckField(raiseError, "opt.raiseError", Is.Boolean(raiseError))
    for try = 1, maxTryCount do
        if try ~= 1 and delay ~= 0 then SleepFor(delay) end

        local result, o = fun(try)
        if result then return result, o end
    end
    if raiseError then error("Reached tries limit") end
    return false
end

return tryFunction
