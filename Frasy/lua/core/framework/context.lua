--- @file    context.lua
--- @author  Paul Thomas
--- @date    3/20/2023
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

Context = {
    info = {
        serial       = "",
        uut          = 0,
        stage        = Stage.idle,
        time         = { start = 0, stop = 0, elapsed = 0, process = 0 },
        orchestrator = {
            version = "1.2.0",
            date = "5/15/2024"
        },
        user         = {
            version = "0.0.0",
            date = "12/31/1969"
        }
    },
    values = {}
}
