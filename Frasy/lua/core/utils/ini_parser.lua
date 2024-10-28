local function Parse(lines)
    local content = {}
    local section = {}
    local key = nil
    for _, line in pairs(lines) do
        line = string.gsub(line, '(;.+)', '')
        for k in string.gmatch(line, '%[(%w+)%]') do
            section = {}
            content[k] = section
        end
        for k, v in string.gmatch(line, "(%w+)=(.+)") do section[k] = v end
    end
    return content
end

return {
    Parse = function(content)
        local lines = LineSplit(content)
        return Parse(lines)
    end,
    LoadFile = function(filename)
        local file, err = io.open(filename, "r")
        if (file) then
            local lines = {}
            ::read_line::
            local content = file:read("l")
            if (content ~= nil) then
                table.insert(lines, content)
                goto read_line
            end
            file:close()
            return Parse(lines)
        else
            error("Failed to open ini file. Error: " .. tostring(err))
        end
    end
}
