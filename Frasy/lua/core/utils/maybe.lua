--- @file    maybe.lua
--- @author  Sam Martel
--- @date    2026-06-03
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

--- Checks if a value is nil or meets a predicate.
--- @param v any? Value to check
--- @param f fun(v: any, ...): boolean Predicate that takes v and returns a boolean
--- @param ... any? Predicate additional parameters (not value), if needed
--- @return boolean
return function(v, f, ...) return type(v) == "nil" or f(v, ...) end
