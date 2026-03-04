add_library(ceell STATIC)

add_subdirectory(osal)
add_subdirectory(core)
add_subdirectory(network)
add_subdirectory(ota)

target_include_directories(ceell PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/osal
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/network
  ${CMAKE_CURRENT_SOURCE_DIR}/ota
)

target_link_libraries(ceell PUBLIC zephyr_interface)
