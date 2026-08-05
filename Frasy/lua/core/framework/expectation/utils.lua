function ExpectToBeTrue(value)
    if Context.info.stage ~= Stage.execution then return { pass = true } end
    local result = {}
    result.method = "ToBeTrue"
    result.expected = true
    result.pass = type(value) == "boolean" and value == result.expected or false
    return result
end

function ExpectToBeFalse(value)
    if Context.info.stage ~= Stage.execution then return { pass = true } end
    local result = {}
    result.method = "ToBeFalse"
    result.expected = false
    result.pass = type(value) == "boolean" and value == result.expected or false
    return result
end

function ExpectToBeEqual(value, expected)
    if Context.info.stage ~= Stage.execution then return { pass = true } end
    local result = {}
    result.method = "ToBeEqual"
    result.expected = expected
    result.pass = type(value) == type(expected) and value == result.expected
    return result
end

function ExpectToBeNear(value, expected, deviation)
    if Context.info.stage ~= Stage.execution then return { pass = true } end
    local result = {}
    result.method = "ToBeNear"
    result.expected = expected
    result.deviation = math.abs(deviation)
    result.min = expected - deviation
    result.max = expected + deviation
    result.pass = type(value) == "number" and (result.min <= value and value <= result.max) or false
    return result
end

function ExpectToBeInRange(value, min, max)
    if Context.info.stage ~= Stage.execution then return { pass = true } end
    local result = {}
    result.method = "ToBeInRange"
    result.min = min
    result.max = max
    result.pass = type(value) == "number" and (min <= value and value <= max) or false
    return result
end

function ExpectToBeInPercentage(value, expected, percentage)
    if Context.info.stage ~= Stage.execution then return { pass = true } end
    local result = {}
    result.method = "ToBeInPercentage"
    result.expected = expected
    result.percentage = percentage
    result.deviation = math.abs(expected * percentage / 100)
    result.min = expected - result.deviation
    result.max = expected + result.deviation
    result.pass = type(value) == "number" and (result.min <= value and value <= result.max) or false
    return result
end

function ExpectToBeGreater(value, min)
    if Context.info.stage ~= Stage.execution then return { pass = true } end
    local result = {}
    result.method = "ToBeGreater"
    result.min = min
    result.pass = type(value) == "number" and value > min or false
    return result
end

function ExpectToBeGreaterOrEqual(value, min)
    if Context.info.stage ~= Stage.execution then return { pass = true } end
    local result = {}
    result.method = "ToBeGreaterOrEqual"
    result.min = min
    result.pass = type(value) == "number" and value >= min or false
    return result
end

function ExpectToBeLesser(value, max)
    if Context.info.stage ~= Stage.execution then return { pass = true } end
    local result = {}
    result.method = "ToBeLesser"
    result.max = max
    result.pass = type(value) == "number" and value < max or false
    return result
end

function ExpectToBeLesserOrEqual(value, max)
    if Context.info.stage ~= Stage.execution then return { pass = true } end
    local result = {}
    result.method = "ToBeLesserOrEqual"
    result.max = max
    result.pass = type(value) == "number" and value <= max or false
    return result
end

function ExpectToBeType(value, expected)
    if Context.info.stage ~= Stage.execution then return { pass = true } end
    local result = {}
    result.method = "ToBeType"
    result.expected = expected
    result.type = type(value)
    result.pass = result.type == expected
    return result
end

function ExpectToBeMatch(value, pattern)
    if Context.info.stage ~= Stage.execution then return { pass = true } end
    local result = {}
    result.method = "ToMatch"
    result.pattern = pattern
    result.pass = type(value) == "string" and string.match(value, pattern) ~= nil or false
    return result
end
