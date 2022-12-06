local HAL = require "hal.lua"

-- Example where 2 UUTs are being tested simultaneously by two different daughterboards.
local panel = {
    -- Lists the UUTs located on a single panel. This allows Frasy to generate the UI of the control panel,
    -- more specifically the fields to input serial numbers, the display of results and spawning of sequences.
    uuts = {"UUT0", "UUT1"},
    -- A global serial number means that only the panel has a serial number. 
    -- If true, Frasy will only take one serial number, no matter how many UUTs are defined.
    globalSerialNumber = false,
    -- Defines the connections between the panel and the daughterboard(s).
    -- A connection can either be unique to a single UUT, where only one physical test point is connected to a single daughterboard,
    -- or a test point can be mapped to multiple daughterboards, or even multiple signals on a single daughterboard.
    -- This is useful in cases where 
    mapping = {
        TP1 = {
            connectedTo = {
                UUT0 = {daughterboardId = "dobo1", signalId = HAL.Point.DBA_MUX1_A0},
                UUT1 = {daughterboardId = "dobo2", signalId = HAL.Point.DBA_MUX1_A0}
            }
        },
        TP2 = {
            connectedTo = {
                UUT0 = {daughterboardId = "dobo1", signalId = HAL.Point.DBA_MUX1_A1},
                UUT1 = {daughterboardId = "dobo2", signalId = HAL.Point.DBA_MUX1_A1}
            }
        },
        TP3 = {
            connectedTo = {
                UUT0 = {daughterboardId = "dobo1", signalId = HAL.Point.DBA_MUX1_A2},
                UUT1 = {daughterboardId = "dobo2", signalId = HAL.Point.DBA_MUX1_A2}
            }
        },
        TP6 = {
            connectedTo = {
                UUT0 = {daughterboardId = "dobo1", signalId = HAL.Point.DBA_MUX1_A3},
                UUT1 = {daughterboardId = "dobo2", signalId = HAL.Point.DBA_MUX1_A3}
            }
        },
        R1 = {
            ImpP = TP1,
            ImpN = TP2
        }
    },
    compartiments = {
        -- empty means no compartimentation.
    }
}


return panel
