local pprint = require('lua.core.utils.pprint')
local linesplit = require('lua.core.utils.linesplit')

local function parse(lines)
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
    parse = function(content)
        local lines = linesplit(content)
        return parse(lines)
    end,
    loadFile = function(filename)
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
            return parse(lines)
        else
            error(err)
        end
    end
}
