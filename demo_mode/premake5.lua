-- Give the relative path to where the Frasy folder is located from this folder
-- When using demo-mode, path should be ".."
-- When using Frasy as a dependency, path could be "vendor/Frasy"
FrasyDir = ".."


workspace "Demo"
    architecture "x64"
    startproject "Frasy"

    configurations {
        "Debug",
        "Release",
        "Dist"
    }

    flags {
        "MultiProcessorCompile",
        "LinkTimeOptimization",
        "Color"
    }

    filter { "toolset:not gcc", "toolset:not clang" }
    buildoptions {
        "/wd5105",
        "/Zc:preprocessor"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

FrasyAbsoluteDir = os.getcwd() .. "/" .. FrasyDir -- Do not edit
include(FrasyAbsoluteDir)

project "Demo"
    kind "ConsoleApp"

    language "C++"
    cppdialect "C++latest"
    staticruntime "On"

    targetdir("bin/" .. outputdir .. "/%{prj.name}")
    objdir("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "src/**.cpp",
    }

    filter "*"
    includedirs {
        FrasyAbsoluteDir .. "/Frasy/src",
        FrasyAbsoluteDir .. "/Frasy/vendor",
        FrasyAbsoluteDir .. "/Brigerad/src",
        FrasyAbsoluteDir .. "/Brigerad/vendor",
        FrasyAbsoluteDir .. "/Brigerad/vendor/spdlog/include",
        FrasyAbsoluteDir .. "/%{IncludeDir.GLFW}",
        FrasyAbsoluteDir .. "/%{IncludeDir.Glad}",
        FrasyAbsoluteDir .. "/%{IncludeDir.ImGui}",
        FrasyAbsoluteDir .. "/%{IncludeDir.glm}",
        FrasyAbsoluteDir .. "/%{IncludeDir.stb_image}",
        FrasyAbsoluteDir .. "/%{IncludeDir.serial}/include",
        FrasyAbsoluteDir .. "/%{IncludeDir.entt}",
        FrasyAbsoluteDir .. "/%{IncludeDir.lua}/src",
        FrasyAbsoluteDir .. "/%{IncludeDir.sol}/include",
        FrasyAbsoluteDir .. "/%{IncludeDir.yaml_cpp}",
        FrasyAbsoluteDir .. "/%{IncludeDir.pfr}",
        FrasyAbsoluteDir .. "/%{IncludeDir.gtest}",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/301",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/303",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/304",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/305",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/309",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/extra",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/storage",
    }

    filter "*"
        defines {
            "CO_MULTIPLE_OD"
        }

    externalincludedirs {
        FrasyAbsoluteDir .. "/Brigerad/vendor/spdlog/include",
        FrasyAbsoluteDir .. "/%{IncludeDir.GLFW}",
        FrasyAbsoluteDir .. "/%{IncludeDir.Glad}",
        FrasyAbsoluteDir .. "/%{IncludeDir.ImGui}",
        FrasyAbsoluteDir .. "/%{IncludeDir.glm}",
        FrasyAbsoluteDir .. "/%{IncludeDir.stb_image}",
        FrasyAbsoluteDir .. "/%{IncludeDir.serial}/include",
        FrasyAbsoluteDir .. "/%{IncludeDir.entt}",
        FrasyAbsoluteDir .. "/%{IncludeDir.lua}/src",
        FrasyAbsoluteDir .. "/%{IncludeDir.sol}/include",
        FrasyAbsoluteDir .. "/%{IncludeDir.yaml_cpp}",
        FrasyAbsoluteDir .. "/%{IncludeDir.pfr}",
        FrasyAbsoluteDir .. "/%{IncludeDir.gtest}",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/301",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/303",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/304",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/305",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/309",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/extra",
        FrasyAbsoluteDir .. "/Frasy/vendor/CANopenNode/storage",
    }

    filter "system:windows"
    includedirs {
        FrasyAbsoluteDir .. "/Frasy/src/platform/windows/can_open"
    }

    externalincludedirs {
        FrasyAbsoluteDir .. "/Frasy/src/platform/windows/can_open"
    }

    filter { "toolset:not gcc", "toolset:not clang" }
    defines {
        "_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING",
        "_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING",
        "_SILENCE_CXX20_CISO646_REMOVED_WARNING",
        "_CRT_SECURE_NO_WARNINGS",
    }
    buildoptions {
        "/wd26812",
        "/wd5105"
    }
    buildoptions {
        "/openmp:experimental"
    }

    filter { "toolset:not gcc", "toolset:not clang", "configurations:Debug" }
    buildoptions {
        "/MTd"
    }

    filter { "toolset:not gcc", "toolset:not clang", "configurations:not Debug" }
    buildoptions {
        "/MT"
    }

    externalwarnings "Off"

    filter "configurations:Debug"
    defines {
        "BR_DEBUG",
        "BR_ENABLE_ASSERTS",
        "BR_PROFILE"
    }
    runtime "Debug"
    symbols "on"

    filter "configurations:Release"
    defines "BR_RELEASE"
    runtime "Release"
    optimize "on"

    filter "configurations:Dist"
    defines "BR_DIST"
    runtime "Release"
    optimize "on"

    filter "system:windows"
    systemversion "latest"

    defines { "BR_PLATFORM_WINDOWS" }

    links {
        "Frasy",
        "Brigerad",
        "GLFW",
        "Glad",
        "ImGui",
        "serial",
        "gtest",
        "opengl32.lib",
        "Ws2_32.lib"
    }

   postbuildcommands {
       "{CMD_COPYDIR} " .. FrasyDir .. "/Frasy/assets bin/" .. outputdir .. "/%{prj.name}/assets",
       "{CMD_RMDIR} bin/" .. outputdir .. "/%{prj.name}/lua",
       "{CMD_COPYDIR} " .. FrasyDir .. "/Frasy/lua/core bin/" .. outputdir .. "/%{prj.name}/lua/core",
       "{CMD_COPYDIR} src/lua/user bin/" .. outputdir .. "/%{prj.name}/lua/user",
       "{CMD_COPYFILE} . bin/"..outputdir .. "/%{prj.name} config.json",
   }
