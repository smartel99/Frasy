--
-- Name:        cmake_project.lua
-- Purpose:     Generate a cmake C/C++ project file.
-- Author:      Ryan Pusztai
-- Modified by: Andrea Zanellato
--              Manu Evans
--              Tom van Dijck
--              Yehonatan Ballas
--              Joel Linn
--              UndefinedVertex
--              Samuel Martel
--              Paul Thomas
-- Created:     2013/05/06
-- Copyright:   (c) 2008-2020 Jason Perkins and the Premake project
--
local p = premake
local tree = p.tree
local project = p.project
local config = p.config
local cmake = p.modules.cmake

cmake.project = {}
local m = cmake.project

local printf = function(s, ...)
    return print(s:format(...))
end

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

--- Check if a file or directory exists in this path
local function exists(file)
    local ok, err, code = os.rename(file, file)
    if not ok then
        if code == 13 then
            -- Permission denied, but it exists
            return true
        end
    end
    return ok, err
end

--- Check if a directory exists in this path
local function isdir(path)
    -- "/" works on both Unix and Windows
    return exists(path .. "/")
end

function m.getcompiler(cfg)
    local default = iif(cfg.system == p.WINDOWS, "msc", "clang")
    local toolset = p.tools[_OPTIONS.cc or cfg.toolset or default]
    if not toolset then
        error("Invalid toolset '" + (_OPTIONS.cc or cfg.toolset) + "'")
    end
    return toolset
end

function m.files(prj)
    local tr = project.getsourcetree(prj)
    tree.traverse(tr, {
        onleaf = function(node, depth)
            _p(depth, "\"%s\"",
                    path.getrelative(prj.workspace.location, node.abspath))
        end
    }, true)
end

--
-- Project: Generate the cmake project file.
--
function m.generate(prj)
    p.utf8()

    -- if kind is only defined for configs, promote to project
    if prj.kind == nil then
        for cfg in project.eachconfig(prj) do
            prj.kind = cfg.kind
        end
    end

    if prj.kind == "Utility" then
        return
    end

    if prj.kind == "StaticLib" then
        _p("add_library(\"%s\" STATIC", prj.name)
    elseif prj.kind == "SharedLib" then
        _p("add_library(\"%s\" SHARED", prj.name)
    else
        if prj.executable_suffix then
            _p("set(CMAKE_EXECUTABLE_SUFFIX \"%s\")", prj.executable_suffix)
        end
        _p("add_executable(\"%s\"", prj.name)
    end
    m.files(prj)
    _p(")")

    _p("\nset(CMAKE_C_STANDARD 11)\nset(CMAKE_CXX_STANDARD 20)\n")

    for cfg in project.eachconfig(prj) do
        local toolset = m.getcompiler(cfg)
        local isclangorgcc = toolset == p.tools.clang or toolset == p.tools.gcc
        _p("if(CMAKE_BUILD_TYPE STREQUAL %s)", cmake.cfgname(cfg))
        -- dependencies
        local dependencies = project.getdependencies(prj)
        if #dependencies > 0 then
            _p(1, "add_dependencies(\"%s\"", prj.name)
            for _, dependency in ipairs(dependencies) do
                _p(2, "\"%s\"", dependency.name)
            end
            _p(1, ")")
        end

        -- output dir
        local ok = isdir(cfg.buildtarget.directory)
        if ok then
        else
            local ok, err = os.mkdir(cfg.buildtarget.directory)
            if ok == nil then
                printf("Unable to create directoy: %s", err)
            end
        end

        _p(1, "set_target_properties(\"%s\" PROPERTIES", prj.name)
        _p(2, "OUTPUT_NAME \"%s\"", cfg.buildtarget.basename)
        _p(2, "ARCHIVE_OUTPUT_DIRECTORY \"%s\"", cfg.buildtarget.directory)
        _p(2, "LIBRARY_OUTPUT_DIRECTORY \"%s\"", cfg.buildtarget.directory)
        _p(2, "RUNTIME_OUTPUT_DIRECTORY \"%s\"", cfg.buildtarget.directory)
        _p(1, ")")
        _p("endif()")

        -- include dirs
        _p("target_include_directories(\"%s\" PRIVATE", prj.name)
        for _, includedir in ipairs(cfg.includedirs) do
            _x(1, "$<$<CONFIG:%s>:%s>", cmake.cfgname(cfg), includedir)
        end
        _p(")")

        -- defines
        _p("target_compile_definitions(\"%s\" PRIVATE", prj.name)
        for _, define in ipairs(cfg.defines) do
            _p(1, "$<$<CONFIG:%s>:%s>", cmake.cfgname(cfg),
                    p.esc(define):gsub(" ", "\\ "))
        end
        _p(")")

        -- lib dirs
        _p("target_link_directories(\"%s\" PRIVATE", prj.name)
        for _, libdir in ipairs(cfg.libdirs) do
            _p(1, "$<$<CONFIG:%s>:%s>", cmake.cfgname(cfg), libdir)
        end
        _p(")")

        -- libs
        _p("target_link_libraries(\"%s\"", prj.name)
        -- Do not use toolset here as cmake needs to resolve dependency chains
        local uselinkgroups = isclangorgcc and cfg.linkgroups == p.ON
        if uselinkgroups then
            _p(1, "-Wl,--start-group")
        end
        for a, link in ipairs(config.getlinks(cfg, "dependencies", "object")) do
            _p(1, "$<$<CONFIG:%s>:%s>", cmake.cfgname(cfg),
                    link.linktarget.basename)
        end
        if uselinkgroups then
            -- System libraries don't depend on the project
            _p(1, "-Wl,--end-group")
            _p(1, "-Wl,--start-group")
        end
        for _, link in ipairs(config.getlinks(cfg, "system", "fullpath")) do
            _p(1, "$<$<CONFIG:%s>:%s>", cmake.cfgname(cfg), link)
        end
        if uselinkgroups then
            _p(1, "-Wl,--end-group")
        end
        _p(")")

        -- setting build options
        all_build_options = ""
        for _, option in ipairs(cfg.buildoptions) do
            all_build_options = all_build_options .. option .. " "
        end

        if all_build_options ~= "" then
            _p("if(CMAKE_BUILD_TYPE STREQUAL %s)", cmake.cfgname(cfg))
            _p(1,
                    "set_target_properties(\"%s\" PROPERTIES COMPILE_FLAGS \"%s\")",
                    prj.name, all_build_options)
            _p("endif()")
        end

        -- setting link options
        all_link_options = ""
        for _, option in ipairs(cfg.linkoptions) do
            all_link_options = all_link_options .. option .. " "
        end

        if all_link_options ~= "" then
            _p("if(CMAKE_BUILD_TYPE STREQUAL %s)", cmake.cfgname(cfg))
            _p(1, "set_target_properties(\"%s\" PROPERTIES LINK_FLAGS \"%s\")",
                    prj.name, all_link_options)
            _p("endif()")
        end

        _p("target_compile_options(\"%s\" PRIVATE", prj.name)
        for _, flag in ipairs(toolset.getcflags(cfg)) do
            _p(1, "$<$<AND:$<CONFIG:%s>,$<COMPILE_LANGUAGE:C>>:%s>",
                    cmake.cfgname(cfg), flag)
        end
        for _, flag in ipairs(toolset.getcxxflags(cfg)) do
            _p(1, "$<$<AND:$<CONFIG:%s>,$<COMPILE_LANGUAGE:CXX>>:%s>",
                    cmake.cfgname(cfg), flag)
        end
        _p(")")

        -- C++ standard
        -- only need to configure it specified
        if (cfg.cppdialect ~= nil and cfg.cppdialect ~= "") or cfg.cppdialect ==
                "Default" then
            local standard = {}
            standard["C++98"] = 98
            standard["C++11"] = 11
            standard["C++14"] = 14
            standard["C++17"] = 17
            standard["C++20"] = 20
            standard["C++latest"] = 23
            standard["gnu++98"] = 98
            standard["gnu++11"] = 11
            standard["gnu++14"] = 14
            standard["gnu++17"] = 17
            standard["gnu++20"] = 20

            local extentions = iif(cfg.cppdialect:find("^gnu") == nil, "NO",
                    "YES")
            local pic = iif(cfg.pic == "On", "True", "False")
            local lto = iif(cfg.flags.LinkTimeOptimization, "True", "False")

            _p("if(CMAKE_BUILD_TYPE STREQUAL %s)", cmake.cfgname(cfg))
            _p(1, "set_target_properties(\"%s\" PROPERTIES", prj.name)
            _p(2, "CXX_STANDARD %s", standard[cfg.cppdialect])
            _p(2, "CXX_STANDARD_REQUIRED YES")
            _p(2, "CXX_EXTENSIONS %s", extentions)
            _p(2, "POSITION_INDEPENDENT_CODE %s", pic)
            _p(2, "INTERPROCEDURAL_OPTIMIZATION %s", lto)
            _p(1, ")")
            _p("endif()")
        end

        -- precompiled headers
        -- copied from gmake2_cpp.lua
        if not cfg.flags.NoPCH and cfg.pchheader then
            local pch = cfg.pchheader
            local found = false

            -- test locally in the project folder first (this is the most likely location)
            local testname = path.join(cfg.project.basedir, pch)
            if os.isfile(testname) then
                pch = project.getrelative(cfg.project, testname)
                found = true
            else
                -- else scan in all include dirs.
                for _, incdir in ipairs(cfg.includedirs) do
                    testname = path.join(incdir, pch)
                    if os.isfile(testname) then
                        pch = project.getrelative(cfg.project, testname)
                        found = true
                        break
                    end
                end
            end

            if not found then
                pch = project.getrelative(cfg.project, path.getabsolute(pch))
            end
            _p("if(CMAKE_BUILD_TYPE STREQUAL %s)", cmake.cfgname(cfg))
            _p("target_precompile_headers(\"%s\" PUBLIC %s/%s)", prj.name,
                    prj.name, pch)
            _p("endif()")
        end

        local getBuildCommands = function(t, cmd)
            local out = ""
            for _, v in ipairs(cfg[t]) do
                local s, e, match = v:find("({CMD_%u*})")
                if s == nil then
                    printf("WARNING: Command '%s...' is not supported...",
                            v:sub(1, 64))
                else
                    v = v:sub(e + 2)
                    v = v:gsub("\\", "/")
                    local tmp = string.format(
                            "add_custom_command(TARGET %s %s\n",
                            prj.name, cmd)

                    -- Check if the file we have is a dir or a file.
                    if match == "{CMD_RMDIR}" then
                        tmp = tmp .. string.format("COMMAND \"${CMAKE_COMMAND}\" -E rm -r -f \"%s/%s\"\n",
                                cfg.project.location, v)
                    elseif match == "{CMD_MKDIR}" then
                        tmp = tmp .. string.format("COMMAND \"${CMAKE_COMMAND}\" -E make_directory \"%s/%s\"\n",
                                cfg.project.location, v)
                    elseif match == "{CMD_COPYDIR}" then
                        local parameters = split_command(v)
                        local source = parameters[1]
                        local destination = parameters[2]
                        if destination == nil then
                            destination = source
                        end
                        tmp = tmp .. string.format(
                                "COMMAND \"${CMAKE_COMMAND}\" -E copy_directory \"%s/%s\" \"%s/%s\"\n",
                                cfg.project.location, source,
                                cfg.project.location, destination)
                    elseif match == "{CMD_COPYFILE}" then
                        local parameters = split_command(v)
                        local source = parameters[1]
                        local destination = parameters[2]
                        local file = parameters[3]
                        if destination == nil then
                            destination = source
                        end
                        if file == nil then
                            error("File can't be empty!")
                        end
                        tmp = tmp ..
                                string.format(
                                        "COMMAND \"${CMAKE_COMMAND}\" -E copy \"%s/%s/%s\" \"%s/%s/%s\"\n",
                                        cfg.project.location, source, file,
                                        cfg.project.location, destination, file)
                    else
                        printf("WARNING: Command '%s...' is not supported...",
                                match)
                        tmp = nil
                    end
                    if tmp ~= nil then
                        out = out .. tmp .. ")\n"
                    end
                    --string.format(
                    --       "COMMENT \"Copied '%s' to target directory\"\n)\n\n",
                    --       v)
                end
            end

            return out
        end

        -- Pre build commands.
        local preBuildCommands = getBuildCommands("prebuildcommands",
                "PRE_BUILD")

        -- Pre link commands.
        local preLinkCommands = getBuildCommands("prelinkcommands", "PRE_LINK")

        -- Post build commands.
        local postBuildCommands = getBuildCommands("postbuildcommands",
                "POST_BUILD")

        if preBuildCommands ~= "" then
            _p("if(CMAKE_BUILD_TYPE STREQUAL %s)", cmake.cfgname(cfg))
            _p("%s", preBuildCommands)
            _p("endif()")
        end
        if preLinkCommands ~= "" then
            _p("if(CMAKE_BUILD_TYPE STREQUAL %s)", cmake.cfgname(cfg))
            _p("%s", preLinkCommands)
            _p("endif()")
        end
        if postBuildCommands ~= "" then
            _p("if(CMAKE_BUILD_TYPE STREQUAL %s)", cmake.cfgname(cfg))
            _p("%s", postBuildCommands)
            _p("endif()")
        end
    end
end
