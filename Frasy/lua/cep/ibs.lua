local ib = require("lua/core/sdk/environment/ib")
local ibs = {}

ibs.daq = ib:new(nil, "daq")
            :Kind(02)
            :NodeId(02)
            :Eds("lua/core/cep/eds/daq.eds")

ibs.pio = ib:new(nil, "pio")
            :Kind(03)
            :NodeId(03)
            :Eds("lua/core/cep/eds/pio_1.0.0.eds")

ibs.r8l = ib:new(nil, "r8l")
            :Kind(04)
            :NodeId(04)
            :Eds("lua/core/cep/eds/r8l_1.0.0.eds")
            :AddFunction("foo", function(self)
                Utils.print(self, 2)
            end)

return ibs
