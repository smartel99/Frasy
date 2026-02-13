set(LUA_DIR "${CMAKE_CURRENT_LIST_DIR}/../lua")
file(GLOB_RECURSE LUA_SOURCES "${LUA_DIR}/src/*.c")
add_library(Lua STATIC
        "${LUA_SOURCES}"
)
target_include_directories(Lua PUBLIC
        ${LUA_DIR}/src
)
target_link_libraries(Lua PRIVATE
        frasy_dep_build_options
)
