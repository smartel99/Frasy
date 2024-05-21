local ib = require("lua/core/sdk/environment/ib")
local ibs = {}

ibs.daq = ib:new()
    :Kind(02)
    :NodeId(02)

ibs.r8l = ib:new()
    :Kind(03)
    :NodeId(03)

ibs.pio = ib:new()
    :Kind(04)
    :NodeId(04)

return ibs
