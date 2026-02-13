set(WKHTMLTOX_DIR "${CMAKE_CURRENT_LIST_DIR}/../wkhtmltox")

add_library(wkhtmltox INTERFACE)
target_link_libraries(wkhtmltox INTERFACE
        ${WKHTMLTOX_DIR}/bin/libwkhtmltox.lib
)
target_include_directories(wkhtmltox INTERFACE
        ${WKHTMLTOX_DIR}/include
)
