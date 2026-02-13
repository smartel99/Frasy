set(WKHTMLTOX_DIR "${CMAKE_CURRENT_LIST_DIR}/../wkhtmltox")

add_library(wkhtmltox INTERFACE)
target_link_libraries(wkhtmltox INTERFACE
        ${WKHTMLTOX_DIR}/lib/libwkhtmltox.lib
)
target_include_directories(wkhtmltox INTERFACE
        ${WKHTMLTOX_DIR}/include
)
add_custom_command(TARGET ${APP_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${WKHTMLTOX_DIR}/bin/wkhtmltox.dll"
        "$<TARGET_FILE_DIR:${APP_NAME}>/wkhtmltox.dll"
)
