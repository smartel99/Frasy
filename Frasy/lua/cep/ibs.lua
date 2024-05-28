local ib = require("lua/core/sdk/environment/ib")
local ibs = {}

ibs.daq = ib:new(nil, "daq")
    :Kind(02)
    :NodeId(02)
    :Eds("lua/core/cep/eds/daq.eds")

ibs.r8l = ib:new(nil, "r8l")
    :Kind(03)
    :NodeId(03)
    :Eds("lua/core/cep/eds/r8l.eds")

ibs.pio = ib:new(nil, "pio")
    :Kind(04)
    :NodeId(04)
    :Eds("lua/core/cep/eds/pio.eds")

return ibs
