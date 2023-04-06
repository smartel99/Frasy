local p = premake

p.modules.frasy = {}
p.modules.frasy._VERSION = p._VERSION

local frasy = p.modules.frasy
local project = p.project

local function split_command(s)
    function find_char(s, c, t, init)
        if t == nil then
            t = {}
        end
        if init == nil then
            init = 1
        end
        index = s:find(c, init, true)
        if index ~= nil then
            table.insert(t, index)
            find_char(s, c, t, index + 1)
        end
        return t
    end

    results = {}
    last = 1
    spaces = find_char(s, " ")
    escapes = find_char(s, "\"")
    for i, space in ipairs(spaces) do
        escaped = false
        for i, v in ipairs(escapes) do
            if (i % 2) == 0 then
                if escapes[i - 1] < space and space < escapes[i] then
                    escaped = true
                end
            end
        end
        if not escaped then
            table.insert(results, s:sub(last, space - 1))
            last = space + 1
        end
    end
    table.insert(results, s:sub(last))
    return results
end

function frasy.generateProject(pjr)
    for cfg in project.eachconfig(pjr) do
        for _, v in ipairs(cfg["postbuildcommands"]) do
            local s, e, match = v:find("({CMD_%u*})")
            if s == nil then
                printf("WARNING: Command '%s...' is not supported...",
                        v:sub(1, 64))
            else
                v = v:sub(e + 2)
                v = v:gsub("\\", "/")
                if match == "{CMD_RMDIR}" then
                    target = string.format("%s/%s", cfg.project.location, v)
                    target = target:gsub("/", "\\")
                    cmd = io.popen("rmdir /S /q " .. target)
                    cmd:close()
                elseif match == "{CMD_MKDIR}" then
                    target = string.format("%s/%s", cfg.project.location, v)
                    target = target:gsub("/", "\\")
                    cmd = io.popen("mkdir " .. target)
                    cmd:close()
                elseif match == "{CMD_COPYDIR}" then
                    local parameters = split_command(v)
                    local source = cfg.project.location .. "/" .. parameters[1]
                    local destination = cfg.project.location .. "/" .. parameters[2]
                    if destination == nil then
                        destination = source
                    end
                    print ("Copy " .. source .. " to " .. destination)
                    cmd = io.popen(string.format("robocopy %s %s /E /MT", source, destination))
                    cmd:close()
                else
                    printf("WARNING: Command '%s...' is not supported...", match)
                end
            end
        end
    end
end

include("_preload.lua")

return frasy