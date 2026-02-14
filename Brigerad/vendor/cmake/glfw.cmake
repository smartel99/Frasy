set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_BUILD_WIN32 ON CACHE BOOL "")
add_subdirectory(vendor/glfw)

if (MSVC)
    target_compile_options(glfw PRIVATE
            /wd4047
            /wd4024
            /wd4013
    )
else ()
    target_compile_options(glfw PRIVATE
            -Wno-misleading-indentation
            -Wno-implicit-function-declaration
            -Wno-implicit-int
            -Wno-int-conversion
            -fpermissive
    )
endif ()
