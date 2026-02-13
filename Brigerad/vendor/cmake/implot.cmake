set(IMPLOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../implot")

add_library(ImPlot STATIC
        "${IMPLOT_DIR}/implot.cpp"
        "${IMPLOT_DIR}/implot_items.cpp"
        "${IMPLOT_DIR}/implot_demo.cpp"
)
target_include_directories(ImPlot SYSTEM PUBLIC
        "${IMPLOT_DIR}"
)
target_link_libraries(ImPlot PRIVATE frasy_dep_build_options)
