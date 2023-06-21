local p = premake

newaction
{
    trigger = "frasy",
    description = "Populate binary output folder with runtime dependencies",

    onProject = function(prj)
        p.modules.frasy.generateProject(prj)
    end,
}

return function(cfg)
    return (_ACTION == "frasy")
end