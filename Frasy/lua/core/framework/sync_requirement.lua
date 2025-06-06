--- @file    requirement.lua
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
local SyncRequirement = {scope = nil, kind = nil}
SyncRequirement.__index = SyncRequirement

SyncRequirement.Kind = {global = 1, ib = 2}

function SyncRequirement:New(scope)
    return setmetatable({scope = scope, kind = SyncRequirement.Kind.global},
                        SyncRequirement)
end

function SyncRequirement:Ib() self.kind = SyncRequirement.Kind.ib end

function SyncRequirement:IsMet() return true end

return SyncRequirement
