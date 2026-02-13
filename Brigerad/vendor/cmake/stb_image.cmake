set(STBIMAGE_DIR "${CMAKE_CURRENT_LIST_DIR}/../stb_image")
add_library(stb_image STATIC
        "${STBIMAGE_DIR}/impl.cpp"
)
target_link_libraries(stb_image PRIVATE
        frasy_dep_build_options
)
target_include_directories(stb_image PUBLIC "${STBIMAGE_DIR}")
