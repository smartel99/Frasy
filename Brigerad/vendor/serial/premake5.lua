-- LuaFormatter off
project "serial"
kind "StaticLib"
language "C++"
staticruntime "On"

targetdir("bin/" .. outputdir .. "/%{prj.name}")
objdir("bin-int/" .. outputdir .. "/%{prj.name}")

includedirs {
    "include"
}

filter { "toolset:not gcc", "toolset:not clang" }
buildoptions {
    "/wd5105",
    "/wd4244"
}
-- disablewarnings
-- {
--     4244, -- 'argument': conversion from 'const wchar_t' to 'const _Elem', possible loss of data
--     5105, -- Fucking scary, https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/c5105?view=msvc-170
-- }

filter "system:windows"
systemversion "latest"

files {
    "src/serial.cc",
    "src/impl/win.cc",
    "src/impl/list_ports/list_ports_win.cc"
}

links {
    "Setupapi.lib"
}

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

-- LuaFormatter on
