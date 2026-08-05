require("lua/core/utils/checkers")
require("lua/core/framework/expectation/utils")

--- @enum ErrorPolicy
ErrorPolicy = {
    stopCurrent = 1,
    stopAll = 2,
}

--- @class ExpectationResult
--- @field value any
--- @field name string
--- @field note string?
--- @field pass boolean? tell if the value fulfilled requirement
--- @field inverted boolean tell if the result should be interpreted with invert logic
--- @field extra any? additional data that could be useful for developer
--- @field method string
--- @field expected any?
--- @field min any?
--- @field max any?
--- @field deviation any?
--- @field percentage any?
--- @field type string?
--- @field Show fun(self)
--- @field ExportAs fun(self, string)

--- @class ExpectationOpt
--- @field note string? Extra note added to the expectation
--- @field extra table? Extra data to be added to the expectation
--- @field onErrorExtra table? Extra data to be added to the expectation if it error occurs
--- @field policy ErrorPolicy? Tell if the expectation should throw on error and how to affect the solution
--- @field inverted boolean? Tell if the result should be interpreted with invert logic, default to false

--- @class Expectation
--- @field value any
--- @field name string
--- @field note string
--- @field extra table?
--- @field onErrorExtra table?
--- @field inverted boolean
--- @field policy ErrorPolicy?
--- @field New fun(any, string, ExpectationOpt?)
--- @field ToBeTrue fun(self): ExpectationResult
--- @field ToBeFalse fun(self): ExpectationResult
--- @field ToBeEqual fun(self, any): ExpectationResult
--- @field ToBeNear fun(self, number, number): ExpectationResult
--- @field ToBeInRange fun(self, number, number): ExpectationResult
--- @field ToBeInPercentage fun(self, number, number): ExpectationResult
--- @field ToBeGreater fun(self, number): ExpectationResult
--- @field ToBeGreaterOrEqual fun(self, number): ExpectationResult
--- @field ToBeLesser fun(self, number): ExpectationResult
--- @field ToBeLesserOrEqual fun(self, number): ExpectationResult
--- @field ToBeType fun(self, string): ExpectationResult
--- @field ToMatch fun(self, string): ExpectationResult

local Expectation = {}
Expectation.__index = Expectation

---@param value any
---@param name string
---@param opt ExpectationOpt?
function Expectation:New(value, name, opt)
    CheckField(name, Is.String)
    local opt = CheckField(opt, Maybe, Is.Table) or {}
    local note = CheckField(opt.note, Maybe, Is.String) or name
    local policy = CheckField(opt.policy, Maybe, Is.IntegerIn, ErrorPolicy.stopCurrent, ErrorPolicy.stopAll)
    local inverted = CheckField(opt.inverted, Maybe, Is.Boolean) or false
    local extra = CheckField(opt.extra, Maybe, Is.Table)
    local onErrorExtra = CheckField(opt.onErrorExtra, Maybe, Is.Table)
    return setmetatable({
        value = value,
        name = name,
        note = note,
        extra = extra,
        onErrorExtra = onErrorExtra,
        inverted = inverted,
        policy = policy,
        __consumed = false,
    }, Expectation)
end

---@deprecated use optional field `inverted` on New instead
---@return Expectation
function Expectation:Not()
    self.inverted = true
    Log.W("Not is deprecated, use optional field `inverted` on New instead")
    return self
end

---@deprecated use optional field `onErrorExtra` on New instead
---@return Expectation
function Expectation:OnErrorExtra(extra)
    self.onErrorExtra = CheckField(extra, Maybe, Is.Table)
    Log.W("OnErrorExtra is deprecated, use optional field `onErrorExtra` on New instead")
    return self
end

local ExpectationResult = {}
ExpectationResult.__index = ExpectationResult

---@param expectation Expectation
---@param result table
function ExpectationResult:New(expectation, result)
    if expectation.__consumed then error(ConsumedExpectation("Expectation has already been consumed by another matcher")) end
    expectation.__consumed = true
    local result = setmetatable({
        value = expectation.value,
        name = expectation.name,
        note = expectation.note,
        pass = result.pass ~= expectation.inverted,
        inverted = expectation.inverted,
        extra = expectation.extra,
        method = result.method,
        expected = result.expected,
        min = result.min,
        max = result.max,
        deviation = result.deviation,
        percentage = result.percentage,
        type = result.type,
    }, ExpectationResult)
    Orchestrator.AddExpectationResult(result)
    if not result.pass then
        if expectation.onErrorExtra then
            result.extra = result.extra or {}
            for k, v in pairs(expectation.onErrorExtra) do result.extra[k] = v end
        end
        if Context.info.stage == Stage.execution and expectation.policy then
            error(UnmetExpectation(expectation.policy))
        end
    end
    return result
end

function Expectation:ToBeTrue()
    return ExpectationResult:New(self, ExpectToBeTrue(self.value))
end

function Expectation:ToBeFalse()
    return ExpectationResult:New(self, ExpectToBeFalse(self.value))
end

function Expectation:ToBeEqual(expected)
    return ExpectationResult:New(self, ExpectToBeEqual(self.value, expected))
end

function Expectation:ToBeNear(expected, deviation)
    return ExpectationResult:New(self, ExpectToBeNear(self.value, expected, deviation))
end

function Expectation:ToBeInRange(min, max)
    return ExpectationResult:New(self, ExpectToBeInRange(self.value, min, max))
end

function Expectation:ToBeInPercentage(expected, percentage)
    return ExpectationResult:New(self, ExpectToBeInPercentage(self.value, expected, percentage))
end

function Expectation:ToBeGreater(min)
    return ExpectationResult:New(self, ExpectToBeGreater(self.value, min))
end

function Expectation:ToBeGreaterOrEqual(min)
    return ExpectationResult:New(self, ExpectToBeGreaterOrEqual(self.value, min))
end

function Expectation:ToBeLesser(max)
    return ExpectationResult:New(self, ExpectToBeLesser(self.value, max))
end

function Expectation:ToBeLesserOrEqual(max)
    return ExpectationResult:New(self, ExpectToBeLesserOrEqual(self.value, max))
end

function Expectation:ToBeType(expected)
    return ExpectationResult:New(self, ExpectToBeType(self.value, expected))
end

function Expectation:ToMatch(pattern)
    return ExpectationResult:New(self, ExpectToBeMatch(self.value, pattern))
end

function ExpectationResult:Show()
    if Context.info.stage == Stage.execution then ShowExpectation(self) end
    return self
end

---@param name string?
function ExpectationResult:ExportAs(name)
    local name = CheckField(name, Maybe, Is.String) or self.name
    Orchestrator.SetValue(Orchestrator.GetScope(), name, self.value)
end

return Expectation
