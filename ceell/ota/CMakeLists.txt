if(CONFIG_CEELL_OTA)
  target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ota_client.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ota_state.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ota_version.c
  )
endif()
