set(TFD_DIR "${CMAKE_CURRENT_LIST_DIR}/../tinyfiledialogs")
add_library(tinyfiledialogs STATIC
        "${TFD_DIR}/tinyfiledialogs.c"
)
target_include_directories(tinyfiledialogs PUBLIC "${TFD_DIR}")
target_link_libraries(tinyfiledialogs PUBLIC
        frasy_dep_build_options
)
