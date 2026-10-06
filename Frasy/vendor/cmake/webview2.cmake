# Microsoft WebView2 SDK, used to render HTML reports to PDF.
# The runtime ships with Windows 11 (and up-to-date Windows 10); only the SDK headers and the static loader are needed.
include(FetchContent)
set(WEBVIEW2_VERSION "1.0.4258.31")
FetchContent_Declare(webview2
        URL "https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/${WEBVIEW2_VERSION}/microsoft.web.webview2.${WEBVIEW2_VERSION}.nupkg"
        URL_HASH SHA256=56f7f4b8bf9aee4b8efefbbdd4f67d5f74ebd1b100ed0806da71bf76af481aa9
        DOWNLOAD_NAME webview2.zip # A .nupkg is a zip, but CMake only extracts known extensions.
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(webview2)

# The package ships a static loader per target architecture.
string(TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" WEBVIEW2_ARCH)
# MSVC reports x64/X86/ARM64; GCC and Clang report their own names.
if (WEBVIEW2_ARCH MATCHES "^(x86_64|amd64)$")
    set(WEBVIEW2_ARCH x64)
elseif (WEBVIEW2_ARCH MATCHES "^i[3-6]86$")
    set(WEBVIEW2_ARCH x86)
elseif (WEBVIEW2_ARCH STREQUAL "aarch64")
    set(WEBVIEW2_ARCH arm64)
endif ()
if (NOT WEBVIEW2_ARCH MATCHES "^(x64|x86|arm64)$")
    message(FATAL_ERROR "WebView2: unsupported target architecture '${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}' (expected x64, x86 or ARM64)")
endif ()

add_library(webview2 INTERFACE)
target_include_directories(webview2 INTERFACE "${webview2_SOURCE_DIR}/build/native/include")
target_link_libraries(webview2 INTERFACE
        "${webview2_SOURCE_DIR}/build/native/${WEBVIEW2_ARCH}/WebView2LoaderStatic.lib"
        version.lib # Required by the static loader.
        Advapi32.lib # Required by the static loader.
        Shlwapi.lib
)
