--- @file    generate.lua
--- @author  Paul Thomas
--- @date    2023-03-15
--- @brief
---
--- @copyright
--- This program is free software: you can redistribute it and/or modify it under the
--- terms of the GNU General Public License as published by the Free Software Foundation, either
--- version 3 of the License, or (at your option) any later version.
--- This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
--- even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
--- General Public License for more details.
--- You should have received a copy of the GNU General Public License along with this program. If
--- not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.

return function(output)
    local error_handler = require("lua/core/framework/error_handler")
    local status, err   = xpcall(function()
        local file = io.open("lua/order.json", "r")
        Orchestrator.SetOrder(Json.decode(file:read("*all")))
        file:close()

        local report   = Orchestrator.Execute()
        report.version = Context.version
        report.uut     = Context.uut
        report.serial  = Context.serial

        file           = io.open(output .. "/" .. Context.uut .. ".json", "w")
        file:write(Json.encode(report))
        file:close()
    end, error_handler)
    if not status then
        if type(err) == "string" then print(err) else print(err.what) end
    end
end
