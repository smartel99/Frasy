set(IMGUI_DIR "${CMAKE_CURRENT_LIST_DIR}/../imgui")

add_library(ImGui STATIC
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
)

target_include_directories(ImGui SYSTEM PUBLIC
        "${IMGUI_DIR}"
)

target_link_libraries(ImGui PRIVATE
        frasy_dep_build_options
)
