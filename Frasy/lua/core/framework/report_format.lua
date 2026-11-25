--- @file    report_format.lua
--- @author  Sam Martel
--- @date    2026-11-25
--- @brief   Layouts of the report given to `onReport` and saved.
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

--- v1: `pass`, `skipped` and `enabled` booleans. v2: `format_version = 2` and `result` strings.
---@enum ReportFormatValue
local ReportFormat = {
    v1 = 1,
    v2 = 2,
}

--- The layout used when a product picks none.
ReportFormat.default = ReportFormat.v1

---@param format any
---@return boolean
function ReportFormat.IsValid(format)
    return format == ReportFormat.v1 or format == ReportFormat.v2
end

return ReportFormat
