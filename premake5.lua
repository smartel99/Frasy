-- LuaFormatter off

function DefineOptions()
    newoption {
        trigger = "src_loc",
        value = "PATH",
        description = "Path (relative or absolute) to the project-specific source files. If not specified, a default demo mode will be used.",
        default = "Frasy/src/demo_mode"
    }

    newoption {
        trigger = "copy_dir",
        value = "DIR",
        description = "Path (relative or absolute) to a directory that should be copied to the output directory upon building the project."
    }
end

function DefineWorkspace()
    workspace "Brigerad"
        architecture "x64"
        startproject "Frasy"

        configurations
        {
            "Debug",
            "Release",
            "Dist"
        }

        flags
        {
            "MultiProcessorCompile",
            "LinkTimeOptimization",
            "Color"
        }

        filter {"toolset:not gcc", "toolset:not clang"}
            buildoptions
            {
                "/wd5105",
                "/Zc:preprocessor"
            }
end

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution folder)
IncludeDir = {}

function CommonFlags()
    filter {"toolset:not gcc", "toolset:not clang"}
        defines
        {
            "_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING",
            "_SILENCE_CXX20_CISO646_REMOVED_WARNING",
            "_CRT_SECURE_NO_WARNINGS",
        }
        buildoptions
        {
            "/wd26812",
            "/wd5105"
        }
        buildoptions
        {
            "/openmp:experimental"
        }

    filter {"toolset:not gcc", "toolset:not clang", "configurations:Debug"}
        buildoptions
        {
            "/MTd"
        }

    filter {"toolset:not gcc", "toolset:not clang", "configurations:not Debug"}
        buildoptions
        {
            "/MT"
        }

    filter "*"
        includedirs
        {
            "Brigerad/vendor/spdlog/include",
            "%{IncludeDir.GLFW}",
            "%{IncludeDir.GLFW}",
            "%{IncludeDir.Glad}",
            "%{IncludeDir.ImGui}",
            "%{IncludeDir.glm}",
            "%{IncludeDir.stb_image}",
            "%{IncludeDir.serial}/include",
            "%{IncludeDir.entt}",
            "%{IncludeDir.lua}/src",
            "%{IncludeDir.sol}/include",
            "%{IncludeDir.yaml_cpp}",
            "%{IncludeDir.pfr}"
        }

        externalincludedirs{
            "Brigerad/vendor/spdlog/include",
            "%{IncludeDir.GLFW}",
            "%{IncludeDir.Glad}",
            "%{IncludeDir.ImGui}",
            "%{IncludeDir.glm}",
            "%{IncludeDir.stb_image}",
            "%{IncludeDir.serial}/include",
            "%{IncludeDir.entt}",
            "%{IncludeDir.lua}/src",
            "%{IncludeDir.sol}/include",
            "%{IncludeDir.yaml_cpp}",
            "%{IncludeDir.pfr}"
        }

        externalwarnings "Off"

    filter "configurations:Debug"
        defines
        {
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
end

function DefineSolution()
    DefineOptions()
    DefineWorkspace()
    IncludeDir["spdlog"] = "Brigerad/vendor/spdlog/include"
    IncludeDir["GLFW"] = "Brigerad/vendor/glfw/include"
    IncludeDir["Glad"] = "Brigerad/vendor/Glad/include"
    IncludeDir["ImGui"] = "Brigerad/vendor/imgui"
    IncludeDir["glm"] = "Brigerad/vendor/glm"
    IncludeDir["stb_image"] = "Brigerad/vendor/stb_image"
    IncludeDir["serial"] = "Brigerad/vendor/serial"
    IncludeDir["entt"] = "Brigerad/vendor/entt/include"
    IncludeDir["lua"] = "Brigerad/vendor/lua"
    IncludeDir["sol"] = "Brigerad/vendor/sol"
    IncludeDir["yaml_cpp"] = "Brigerad/vendor/yaml-cpp/include"
    IncludeDir["gtest"] = _MAIN_SCRIPT_DIR.."/Brigerad/vendor/googletest/googletest"
    IncludeDir["pfr"] = _MAIN_SCRIPT_DIR .. "/Brigerad/vendor/pfr/include"

    group "Dependencies"
        include "Brigerad/vendor/GLFW"
        include "Brigerad/vendor/Glad"
        include "Brigerad/vendor/ImGui"
        include "Brigerad/vendor/lua"
        include "Brigerad/vendor/yaml-cpp"
        include "Brigerad/vendor/serial"

    group "Tests"
        include "Brigerad/vendor/googletest"

    group ""
    DefineBrigerad()
    DefineFrasy()
end

function DefineBrigerad()
    project "Brigerad"
        location "Brigerad"
        kind "StaticLib"
        language "C++"
        cppdialect "C++20"
        staticruntime "On"

        targetdir ("bin/" .. outputdir .. "/%{prj.name}")
        objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

        files
        {
            "%{prj.name}/src/**.h",
            "%{prj.name}/src/**.cpp",
            "%{prj.name}/vendor/stb_image/**.cpp",
            "%{prj.name}/vendor/stb_image/**.h",
            "%{prj.name}/vendor/glm/**.hpp",
            "%{prj.name}/vendor/glm/**.inl",
            "%{prj.name}/vendor/spdlog/include/**.h",
            -- "%{prj.name}/vendor/spdlog/src/**.cpp",
        }

        includedirs
        {
            "%{prj.name}/src",
        }

        CommonFlags()

        filter "system:windows"
            systemversion "latest"

            defines
            {
                "BR_PLATFORM_WINDOWS",
                "BR_BUILD_DLL",
                "GLFW_INCLUDE_NONE"
            }

            links
            {
                "GLFW",
                "Glad",
                "ImGui",
                "lua",
                "serial",
                "opengl32.lib",
                "yaml-cpp"
            }

            excludes
            {
                "%{prj.name}/src/Platform/Linux/**.h",
                "%{prj.name}/src/Platform/Linux/**.cpp",
            }
end

function DefineFrasy()
project "Frasy"
    location "Frasy"
    kind "ConsoleApp"

    language "C++"
    cppdialect "C++20"
    staticruntime "On"

    targetdir("bin/" .. outputdir .. "/%{prj.name}")
    objdir("bin-int/" .. outputdir .. "/%{prj.name}")

    if _OPTIONS.src_loc == "Frasy/src/layers/demo_mode" then
        print("--src_loc not provided, building in demo mode")
    elseif _OPTIONS.src_loc == "" then
        error("--src_loc cannot be empty!")
    end

    files {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/vendor/**.h",
        "%{prj.name}/vendor/**.cpp",
        "%{prj.name}/vendor/**.c",
        _OPTIONS.src_loc .. "/**.h",
        _OPTIONS.src_loc .. "/**.hpp",
        _OPTIONS.src_loc .. "/**.c",
        _OPTIONS.src_loc .. "/**.cpp",
    }

    excludes {"%{prj.name}/test/**.h", "%{prj.name}/test/**.cpp" }


    includedirs {
        "%{prj.name}/src",
        "Brigerad/vendor",
        "Brigerad/src",
        "%{prj.name}/vendor"
    }

    CommonFlags()

    filter "system:windows"
        systemversion "latest"

        defines {"BR_PLATFORM_WINDOWS"}

        links {
            "Brigerad",
            "GLFW",
            "Glad",
            "ImGui",
            "serial",
            "gtest",
            "opengl32.lib",
            "Ws2_32.lib"
        }

        prebuildcommands {"python.exe incrementVersion.py"}
        postbuildcommands {
            -- "{COPY} vendor\\lua54\\bin\\lua54.dll ..\\bin\\" .. outputdir .. "\\%{prj.name} /K /I /Y",
            "{COPYDIR} assets",
        }

        if _OPTIONS.copy_dir ~= "" and _OPTIONS.copy_dir ~= nil then
            print("Copying '" .. _OPTIONS.copy_dir .. "' to bin output directory")
            postBuildCommands {
                "{COPY} " .. _OPTIONS.copy_dir .. " ..\\bin\\" .. outputdir .. "\\%{prj.name}\\" .. _OPTIONS.copy_dir
            }
        end
end

DefineSolution()
-- LuaFormatter on
