return function(content)
    local lines = {}
    for line in string.gmatch(content, "^[\r\n]+") do
        table.insert(lines, line)
    end
    return lines
end
