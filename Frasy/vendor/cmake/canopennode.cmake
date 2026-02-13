set(CANOPEN_DIR "${CMAKE_CURRENT_LIST_DIR}/../CANopenNode")
file(GLOB_RECURSE CANOPEN_SOURCES
        "${CANOPEN_DIR}/CANopen.c"
        "${CANOPEN_DIR}/301/*.c"
        "${CANOPEN_DIR}/303/*.c"
        "${CANOPEN_DIR}/304/*.c"
        "${CANOPEN_DIR}/305/*.c"
        "${CANOPEN_DIR}/309/*.c"
        "${CANOPEN_DIR}/extra/*.c"
        "${CANOPEN_DIR}/storage/CO_storage.c"
)

file(GLOB_RECURSE CANOPEN_APP_SOURCES
        "${CANOPEN_APP_DIR}/*.c"
        "${CANOPEN_APP_DIR}/*.cpp"
)
message(STATUS "CANOPEN_APP_SOURCES: ${CANOPEN_APP_SOURCES}")

add_library(CanOpen STATIC
        ${CANOPEN_SOURCES}
        ${CNAOPEN_APP_SOURCES}
        ${CANOPEN_OD_DIR}/OD.c
)
target_link_libraries(CanOpen PRIVATE
        frasy_dep_build_options
)
target_include_directories(CanOpen PUBLIC
        "${CANOPEN_APP_DIR}"
        "${CANOPEN_OD_DIR}"
)
target_include_directories(CanOpen PUBLIC
        "${CANOPEN_DIR}"
        "${CANOPEN_DIR}/301"
        "${CANOPEN_DIR}/303"
        "${CANOPEN_DIR}/304"
        "${CANOPEN_DIR}/305"
        "${CANOPEN_DIR}/309"
        "${CANOPEN_DIR}/extra"
        "${CANOPEN_DIR}/storage"
)
target_compile_definitions(CanOpen PUBLIC -DCO_MULTIPLE_OD)
