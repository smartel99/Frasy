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

add_library(webview2 INTERFACE)
target_include_directories(webview2 INTERFACE "${webview2_SOURCE_DIR}/build/native/include")
target_link_libraries(webview2 INTERFACE
        "${webview2_SOURCE_DIR}/build/native/x64/WebView2LoaderStatic.lib"
        version.lib # Required by the static loader.
        Advapi32.lib # Required by the static loader.
        Shlwapi.lib
)
