set(JSON_DIR "${CMAKE_CURRENT_LIST_DIR}/../nlohmann")
add_library(json INTERFACE)
target_include_directories(json INTERFACE "${JSON_DIR}")
