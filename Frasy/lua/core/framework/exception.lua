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
    "UnmetRequirement", -- 1
    "UnmetExpectation", -- 2
    "NestedScope", -- 3
    "BadScope", -- 4
    "AlreadyDefined", -- 5
    "NotFound", -- 6
    "InvalidRequirement", -- 7
    "InvalidStage", -- 8
    "MapperError", -- 9
    "TeamError", -- 10
    "WorkerError", -- 11
    "GenerationError", -- 12
    "GenericError", -- 13
}

local exceptions_values = {}
for index, what in pairs(exceptions) do
    exceptions_values[what] = {
        code = index,
        what = what
    }
end

local function PopulateException(exception, what)
    return {
        code = exception.code,
        what = exception.what .. (what ~= nil and (": " .. what) or "")
    }
end

function UnmetRequirement(what) return PopulateException(exceptions_values.UnmetRequirement, what) end
function UnmetExpectation(what) return PopulateException(exceptions_values.UnmetExpectation, what) end
function NestedScope(what) return PopulateException(exceptions_values.NestedScope, what) end
function BadScope(what) return PopulateException(exceptions_values.BadScope, what) end
function AlreadyDefined(what) return PopulateException(exceptions_values.AlreadyDefined, what) end
function NotFound(what) return PopulateException(exceptions_values.NotFound, what) end
function InvalidRequirement(what) return PopulateException(exceptions_values.InvalidRequirement, what) end
function InvalidStage(what) return PopulateException(exceptions_values.InvalidStage, what) end
function MapperError(what) return PopulateException(exceptions_values.MapperError, what) end
function TeamError(what) return PopulateException(exceptions_values.TeamError, what) end
function WorkerError(what) return PopulateException(exceptions_values.WorkerError, what) end
function GenerationError(what) return PopulateException(exceptions_values.GenerationError, what) end
function GenericError(what)
    if(what == nil) then what = exceptions_values.GenericError.what end
    return {
        code = exceptions_values.GenericError.code,
        what = what
    }
end
