add_library(imspinner INTERFACE)

target_include_directories(imspinner SYSTEM INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../imspinner
)
