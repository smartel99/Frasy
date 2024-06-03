local function pprint(t, lvl, max_depth)
    if (type(t) == "table") then
        local indent = ""
        for i = 1, lvl do indent = indent .. "  " end
        local str = ""
        local first = true
        for k, v in pairs(t) do
            if (not first or lvl ~= 0) then str = str .. "\n" end
            if (first) then first = false end
            if (type(max_depth) == 'number' and max_depth == lvl) then
                str = str .. indent .. tostring(k) .. ": " .. tostring(v)
            else
                str = str .. indent .. tostring(k) .. ": " ..
                          pprint(v, lvl + 1, max_depth)
            end
        end
        return str
    elseif (type(t) == "function" or type(t) == "thread" or type(t) ==
        "userdata") then
        return type(t)
    else
        return tostring(t)
    end
end
return pprint
