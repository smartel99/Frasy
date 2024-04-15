local Testbench = require "testbench.lua"

function Sequence(name, func)
    local sequence = {
        name = name,
        tests = {},
        LoadTests = func,
        doTests = function(self)
            for i, test in ipairs(self.tests) do test:Run() end
        end,
        GetTestTable = function(self, testName)
            assert(self.tests[testName] ~= nil)
            return self.tests[testName]
        end,
        TestOnValues = function(self, testName, testFunc, params)
            assert(type(testName) == "string")
            if type(testFunc) == "nil" then return self.GetTestTable(name) end

            assert(type(params) == "table")
            table.insert(self.tests,
                         {name = testName, func = testFunc, params = params, Requires})

            return self.tests[#self.tests] -- last element created.
        end
    }

    sequence:LoadTests()

    return sequence
end

Sequence("Test Sequence", function(self)
    Test("Impedances", function(params)
        local impedance =
            Testbench.GetImpedance(params.Point, params.Range, 1.0)
        -- Allow 10% of deviation, injecting 1V.
        Expect(impedance).ToBeNear(params.Expected, 0.10)
    end, {
        -- Expects impedance VCC-GND to be ~1M.
        -- Expects impedance VCCB-GND to be ~100k.
        -- Expects impedance VBAT-GND to be ~10M.
        {Point = map.VCC, Expected = 1.0 * 10 ^ 6, Range = "1M"},
        {Point = map.VCCB, Expected = 1.0 * 10 ^ 5, Range = "100k"},
        {Point = map.VBAT, Expected = 1.0 * 10 ^ 7, Range = "1M"}
    })

    Test("Program UUT", function()
        local result = Testbench.ProgramUUT()
        Expect(result).ToBeTrue()
    end):Requires(Test("Impedances").ToPass(), State.Power.IsOn())

    Test("Read Tension", function()
        local tension = Testbench.GetTension("TP1")
        -- Expects 1V +/- 1%.
        Expect(tension).ToBeNear(1.0, 0.01)
    end):Requires(Test("Impedances"):ToPass(),
                  Test("Program UUT"):ToBeCompleted())
end)
