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

-- Include directories relative to root folder (solution folder)
IncludeDir = {}

function CANopenIncludes()
    includedirs {
        "Frasy/vendor/CANopenNode",
        "Frasy/vendor/CANopenNode/301",
        "Frasy/vendor/CANopenNode/303",
        "Frasy/vendor/CANopenNode/304",
        "Frasy/vendor/CANopenNode/305",
        "Frasy/vendor/CANopenNode/309",
        "Frasy/vendor/CANopenNode/extra",
        "Frasy/vendor/CANopenNode/storage",
    }

    externalincludedirs {
        "Frasy/vendor/CANopenNode",
        "Frasy/vendor/CANopenNode/301",
        "Frasy/vendor/CANopenNode/303",
        "Frasy/vendor/CANopenNode/304",
        "Frasy/vendor/CANopenNode/305",
        "Frasy/vendor/CANopenNode/309",
        "Frasy/vendor/CANopenNode/extra",
        "Frasy/vendor/CANopenNode/storage",
    }

    filter "system:windows"
        includedirs {
            "Frasy/src/platform/windows/can_open"
        }

        externalincludedirs {
            "Frasy/src/platform/windows/can_open"
        }

    filter "*"
        defines {
            "CO_MULTIPLE_OD"
        }
end

function CommonFlags()
    filter { "toolset:not gcc", "toolset:not clang" }
    defines {
        "_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS",
        "_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING",
        "_SILENCE_CXX20_CISO646_REMOVED_WARNING",
        "_CRT_SECURE_NO_WARNINGS",
    }
    buildoptions {
        "/wd26812",
        "/wd5105"
    }
    buildoptions {
        "/openmp:experimental",
        "/permissive-"
    }

    filter { "toolset:not gcc", "toolset:not clang", "configurations:Debug or RelWithDebInfo" }
    buildoptions {
        "/MTd"
    }

    filter { "toolset:not gcc", "toolset:not clang", "configurations:not Debug" }
    buildoptions {
        "/MT"
    }

    filter "*"
    includedirs {
        "Brigerad/vendor/spdlog/include",
        "Brigerad/vendor/tinyfiledialogs",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.ImPlot}",
        "Brigerad/vendor/imspinner",
        "%{IncludeDir.glm}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.serial}/include",
        "%{IncludeDir.entt}",
        "%{IncludeDir.lua}/src",
        "%{IncludeDir.sol}/include",
        "%{IncludeDir.yaml_cpp}",
        "%{IncludeDir.pfr}",
        "%{IncludeDir.gtest}",
    }

    externalincludedirs {
        "Brigerad/vendor/spdlog/include",
        "Brigerad/vendor/tinyfiledialogs",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.Glad}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.ImPlot}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.serial}/include",
        "%{IncludeDir.entt}",
        "%{IncludeDir.lua}/src",
        "%{IncludeDir.sol}/include",
        "%{IncludeDir.yaml_cpp}",
        "%{IncludeDir.pfr}",
        "%{IncludeDir.gtest}",
    }

    CANopenIncludes()

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

    filter "configurations:RelWithDebInfo"
    defines "BR_RELEASE"
    runtime "Release"
    optimize "on"
    symbols "on"

    filter "configurations:Dist"
    defines "BR_DIST"
    runtime "Release"
    optimize "on"
end

function DefineSolution()
    DefineOptions()
    IncludeDir["spdlog"] = "Brigerad/vendor/spdlog/include"
    IncludeDir["GLFW"] = "Brigerad/vendor/glfw/include"
    IncludeDir["Glad"] = "Brigerad/vendor/Glad/include"
    IncludeDir["ImGui"] = "Brigerad/vendor/imgui"
    IncludeDir["ImPlot"] = "Brigerad/vendor/implot"
    IncludeDir["glm"] = "Brigerad/vendor/glm"
    IncludeDir["stb_image"] = "Brigerad/vendor/stb_image"
    IncludeDir["serial"] = "Brigerad/vendor/serial"
    IncludeDir["entt"] = "Brigerad/vendor/entt/include"
    IncludeDir["lua"] = "Brigerad/vendor/lua"
    IncludeDir["sol"] = "Brigerad/vendor/sol"
    IncludeDir["yaml_cpp"] = "Brigerad/vendor/yaml-cpp/include"
    IncludeDir["gtest"] =  "Brigerad/vendor/googletest/googletest/include"
    IncludeDir["pfr"] =  "Brigerad/vendor/pfr/include"
    IncludeDir["CANopen"] = "Frasy/vendor/"

    group "Dependencies"
    include "Brigerad/vendor/GLFW"
    include "Brigerad/vendor/Glad"
    include "Brigerad/vendor/ImGui"
    include "Brigerad/vendor/ImPlot"
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
    cppdialect "C++latest"
    staticruntime "On"

    targetdir("bin/" .. outputdir .. "/%{prj.name}")
    objdir("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/vendor/stb_image/**.cpp",
        "%{prj.name}/vendor/stb_image/**.h",
        "%{prj.name}/vendor/glm/**.hpp",
        "%{prj.name}/vendor/glm/**.inl",
        "%{prj.name}/vendor/spdlog/include/**.h",
        "%{prj.name}/vendor/tinyfiledialogs/tinyfiledialogs.c",
        "%{prj.name}/vendor/tinyfiledialogs/tinyfiledialogs.h"
        -- "%{prj.name}/vendor/spdlog/src/**.cpp",
    }

    includedirs {
        "%{prj.name}/src",
    }

    CommonFlags()

    filter "system:windows"
    systemversion "latest"

    defines {
        "BR_PLATFORM_WINDOWS",
        "BR_BUILD_DLL",
        "GLFW_INCLUDE_NONE",
        "UNICODE"
    }

    links {
        "GLFW",
        "Glad",
        "ImGui",
        "ImPlot",
        "lua",
        "serial",
        "opengl32.lib",
        "yaml-cpp",
        "Winmm.lib"
    }

    excludes {
        "%{prj.name}/src/Platform/Linux/**.h",
        "%{prj.name}/src/Platform/Linux/**.cpp",
    }
end

function DefineFrasy()
    project "Frasy"
    location "Frasy"
    kind "StaticLib"

    language "C++"
    cppdialect "C++latest"
    staticruntime "On"

    targetdir("bin/" .. outputdir .. "/%{prj.name}")
    objdir("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "%{prj.name}/src/**.cpp",
        "%{prj.name}/src/utils/communication/can_open/OD.c",
        "%{prj.name}/vendor/hashdir/src/hashdir.cpp",
    }

    files {
        "%{prj.name}/vendor/CANopenNode/CANopen.c",
        "%{prj.name}/vendor/CANopenNode/301/**.c",
        "%{prj.name}/vendor/CANopenNode/303/**.c",
        "%{prj.name}/vendor/CANopenNode/304/**.c",
        "%{prj.name}/vendor/CANopenNode/305/**.c",
        "%{prj.name}/vendor/CANopenNode/309/**.c",
        "%{prj.name}/vendor/CANopenNode/extra/**.c",
        "%{prj.name}/vendor/CANopenNode/storage/CO_storage.c",
    }

    removefiles { "%{prj.name}/test/**.h", "%{prj.name}/test/**.cpp" }

    includedirs {
        "%{prj.name}/src",
        "Brigerad/vendor",
        "Brigerad/src",
        "%{prj.name}/vendor",
        "%{prj.name}/vendor/hashdir/include",
        "%{prj.name}/vendor/hashdir/vendor/highwayhash"
    }

    CANopenIncludes()

    CommonFlags()

    filter "system:windows"
    systemversion "latest"

    files {
        "Frasy/src/platform/windows/can_open/**.c",
        "Frasy/src/platform/windows/can_open/**.cpp",
    }

    defines {
        "BR_PLATFORM_WINDOWS"
    }

    links {
        "Brigerad",
        "GLFW",
        "Glad",
        "ImGui",
        "ImPlot",
        "serial",
        "gtest",
        "opengl32.lib",
        "Ws2_32.lib"
    }
end

DefineSolution()
-- LuaFormatter on
