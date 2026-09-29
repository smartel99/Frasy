cmake_minimum_required(VERSION 3.22)
project(wkhtmltopdf CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(wkhtmltox SHARED IMPORTED)
set_target_properties(wkhtmltox PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/bin/wkhtmltox.dll"
        IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/bin/libwkhtmltox.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(wkhtmltopdf main.cpp)
target_link_libraries(wkhtmltopdf PRIVATE wkhtmltox)

# Copy the DLL next to the executable so the build output is directly portable.
add_custom_command(TARGET wkhtmltopdf POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:wkhtmltox> $<TARGET_FILE_DIR:wkhtmltopdf>)
