--- @file    exception.lua
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

local exceptions        = {
    "UnmetRequirement",
    "UnmetExpectation",
    "NestedScope",
    "BadScope",
    "AlreadyDefined",
    "NotFound",
    "InvalidRequirement",
    "InvalidStage",
    "MapperError",
    "TeamError",
    "WorkerError",
    "GenericError",
}

local exceptions_values = {}
for index, what in pairs(exceptions) do
    exceptions_values[what] = {
        code = index,
        what = what
    }
end

local function populationException(exception, what)
    return {
        code = exception.code,
        what = exception.what .. (what ~= nil and (": " .. what) or "")
    }
end

function UnmetRequirement(what) return populationException(exceptions_values.UnmetRequirement, what) end
function UnmetExpectation(what) return populationException(exceptions_values.UnmetExpectation, what) end
function NestedScope(what) return populationException(exceptions_values.NestedScope, what) end
function BadScope(what) return populationException(exceptions_values.BadScope, what) end
function AlreadyDefined(what) return populationException(exceptions_values.AlreadyDefined, what) end
function NotFound(what) return populationException(exceptions_values.NotFound, what) end
function InvalidRequirement(what) return populationException(exceptions_values.InvalidRequirement, what) end
function InvalidStage(what) return populationException(exceptions_values.InvalidStage, what) end
function MapperError(what) return populationException(exceptions_values.MapperError, what) end
function TeamError(what) return populationException(exceptions_values.TeamError, what) end
function WorkerError(what) return populationException(exceptions_values.WorkerError, what) end
function GenericError(what)
    return {
        code = exceptions_values.GenericError.code,
        what = (what ~= nil and what or exceptions_values.GenericError.what)
    }
end