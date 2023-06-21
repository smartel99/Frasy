--- @file    sectionize.lua
--- @author  Paul Thomas
--- @date    5/9/2023
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

Log = {}
function Log.i(message)
    print(message)
end

local Sort            = require("lua/framework/sort_utils")
local Utils           = {}
Utils.print           = require("lua/utils/print")

local sequences       = {
    { "S1", "S2" },
    { "S3", "S4" },
    { "S5", "S6" },
}

local tests           = {
    S1 = { { "T1", "T2" }, { "T3", "T4" }, { "T5", "T6" } },
    S2 = { { "T1", "T2" }, { "T3", "T4" }, { "T5", "T6" } },
    S3 = { { "T1", "T2" }, { "T3", "T4" }, { "T5", "T6" } },
    S4 = { { "T1", "T2" }, { "T3", "T4" }, { "T5", "T6" } },
    S5 = { { "T1", "T2" }, { "T3", "T4" }, { "T5", "T6" } },
    S6 = { { "T1", "T2" }, { "T3", "T4" }, { "T5", "T6" } },
}

local sectionized     = { sequences = {}, tests = {} }
sectionized.sequences = Sort.Sectionize(sequences, { S1 = 0, S6 = 0 })
sectionized.tests.S1  = Sort.Sectionize(tests.S1, { T1 = 0, T6 = 0 })
sectionized.tests.S2  = Sort.Sectionize(tests.S2, {  })
sectionized.tests.S3  = Sort.Sectionize(tests.S3, {  })
sectionized.tests.S4  = Sort.Sectionize(tests.S4, {  })
sectionized.tests.S5  = Sort.Sectionize(tests.S5, {  })
sectionized.tests.S6  = Sort.Sectionize(tests.S6, {  })

local json = require("lua/vendor/json")
print(json.encode(Sort.CombineSectionized(sectionized.sequences, sectionized.tests)))

