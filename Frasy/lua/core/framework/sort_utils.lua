--- @file    sort_utils.lua
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

--- functions used by the orchestrator to sort scopes
local Sort  = {}

--- Add an edge requirement to our edges structure
--- Check if the edge is valid then add it to the structure
function Sort.AddEdgeRequirement(edge, requirement)
    if requirement.reference ~= nil then error(InvalidRequirement()) end
    if requirement.scope.test ~= nil then
        if edge.tests[requirement.scope.sequence] ~= nil then error(InvalidRequirement()) end
        edge.tests[requirement.scope.sequence] = requirement.scope.test
    else
        if edge.sequence ~= nil then error(InvalidRequirement()) end
        edge.sequence = requirement.scope.sequence
    end
end

--- Add an edge scope to the order list
--- Will add a layer with a single scope into the order list
--- Used to add scope with last and first requirements
--- Must be used at the right time in order to meet the first and last requirements
function Sort.AddEdgeToOrder(edge, order)
    if edge ~= nil then table.insert(order, { edge }) end
end

--- Check if the scope marked as last can actually be used as last
--- @param last string scope to be executed last
--- @param
function Sort.CheckLastIsValid(last, dependencies)
    if last == nil then return end
    for owner, references in ipairs(dependencies) do
        if owner ~= last then
            for reference, _ in ipairs(references) do
                if reference == last then error(InvalidRequirement()) end
            end
        end
    end
end

function Sort.HasMatDependencies(requirement, scopes)
    for dep, _ in pairs(requirement) do
        for _, scope in ipairs(scopes) do
            if scope == dep then return false end
        end
    end
    return true
end

--- Sort a list of scope based on their requirements
--- @param scope table list of scopes to sort
--- @param first string name of the scope to run first, can be nil
--- @param last string name of the scope to run last, can be nil
--- @param requirements table list of scopes dependencies
function Sort.SortScopes(scopes, first, last, requirements)
    if scopes == nil then return { } end

    local order = {}

    Sort.CheckLastIsValid(last, requirements)

    -- deletion safe loop
    for index = #scopes, 1, -1 do
        local name = scopes[index]
        if name == first or name == last then table.remove(scopes, index) end
    end

    Sort.AddEdgeToOrder(first, order)

    while (#scopes ~= 0) do
        local layer = {}
        local removal = {}
        -- deletion safe loop
        for index, name in ipairs(scopes) do
            if Sort.HasMatDependencies(requirements[name], scopes) then
                table.insert(layer, name)
                table.insert(removal, index)
            end
        end

        for index = #removal, 1, -1 do
            table.remove(scopes, removal[index])
        end
        if #layer == 0 then error(InvalidRequirement()) end
        table.insert(order, layer)
    end

    Sort.AddEdgeToOrder(last, order)
    return order
end

return Sort