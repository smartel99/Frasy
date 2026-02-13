set(SERIAL_DIR "${CMAKE_CURRENT_LIST_DIR}/../serial")

add_library(serial STATIC
        "${SERIAL_DIR}/src/serial.cc"
        "${SERIAL_DIR}/src/impl/win.cc"
        "${SERIAL_DIR}/src/impl/list_ports/list_ports_win.cc"
)
if(MSVC)
    target_compile_options(serial PRIVATE
            /wd4005 # macro redefinition (common for "alloca" redefined)
            /wd4244 # conversion from 'type1' to 'type2', possible loss of data
    )
else()
    target_compile_options(serial PRIVATE -Wno-macro-redefined) # Clang
    # GCC doesn't have -Wmacro-redefined in the same way; you'd use -Wno-error=cpp or similar if needed
endif()

target_link_libraries(serial PRIVATE
        frasy_dep_build_options
        Setupapi.lib
)
target_include_directories(serial SYSTEM PUBLIC "${SERIAL_DIR}/include")
