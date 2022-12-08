-- LuaFormatter off
project "gtest"
    kind "StaticLib"
    language "C++"

    cppdialect "C++20"

    -- local outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
    targetdir("bin/"..outputdir.."/%{prj.name}")
    objdir("bin-int/"..outputdir.."/%{prj.name}")

    includedirs {
        "googletest",
        "googletest/include",
        "googletest/include/gtest",
    }

    files
    {
        "googletest/**.h",
        "googletest/**.hpp",
        "googletest/src/gtest-all.cc"
    }

    filter "system:windows"
        systemversion "latest"
        staticruntime "On"


    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"

-- LuaFormatter on
