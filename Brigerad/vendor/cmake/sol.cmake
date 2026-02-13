set(SOL_DIR "${CMAKE_CURRENT_LIST_DIR}/../sol")
add_library(Sol INTERFACE)
target_include_directories(Sol INTERFACE "${SOL_DIR}/include")
