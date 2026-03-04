if(CONFIG_CEELL_OTA)
  target_sources(ceell PRIVATE
    ota_client.c
    ota_state.c
    ota_version.c
  )
endif()
