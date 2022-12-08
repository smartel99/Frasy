-- LuaFormatter off
project "serial"
    kind "StaticLib"
    language "C++"
    staticruntime "off"

    targetdir("bin/" .. outputdir .. "/%{prj.name}")
    objdir("bin-int/" .. outputdir .. "/%{prj.name}")

    includedirs 
    {
        "include"
    }

    filter {"toolset:not gcc", "toolset:not clang"}
        buildoptions
        {
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

        files 
        {
            "src/serial.cc",
            "src/impl/win.cc",
            "src/impl/list_ports/list_ports_win.cc"
        }

-- LuaFormatter on
