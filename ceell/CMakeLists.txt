add_library(ceell STATIC)

add_subdirectory(core)
add_subdirectory(network)

target_include_directories(ceell PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/network
)

target_link_libraries(ceell PUBLIC zephyr_interface)
