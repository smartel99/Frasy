set(CPP_HTTPLIB_DIR "${CMAKE_CURRENT_LIST_DIR}/../cpp-httplib")
add_library(cpp-httplib INTERFACE)
target_include_directories(cpp-httplib INTERFACE "${CPP_HTTPLIB_DIR}")
# cpp-httplib requires Ws2_32 on Windows for socket operations
if (WIN32)
    target_link_libraries(cpp-httplib INTERFACE Ws2_32.lib)
endif ()
