local CtrlRoom = {}

function CtrlRoom.RenderUserInputs(self)
    -- Function that renders the user inputs for the project.
    -- These inputs may include the serial number(s), board-specific options, etc.
    -- This function is called before the Run button.
end

function CtrlRoom.RenderTestResults(self, results)
    -- Function that renders the test results.
    -- The results table is a list of KVPs, where the key is the name of the UUT, and the value is a flag indicating its status.
    -- This function is called after the Run button.
end

function CtrlRoom.GetTestData(self)
    -- Gets the path of the test script to execute, as well as the additional data to feed to the test script.
    return self.scriptPath, self.testData
end

return CtrlRoom
