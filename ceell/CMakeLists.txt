add_subdirectory(osal)
add_subdirectory(core)
add_subdirectory(network)
if(CONFIG_CEELL_OTA)
  add_subdirectory(ota)
endif()

zephyr_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/osal
  ${CMAKE_CURRENT_SOURCE_DIR}/core
  ${CMAKE_CURRENT_SOURCE_DIR}/network
  ${CMAKE_CURRENT_SOURCE_DIR}/ota
)
