if(CONFIG_CEELL_OTA)
  zephyr_library()

  zephyr_library_sources(
    ota_client.c
    ota_state.c
    ota_version.c
  )
endif()
