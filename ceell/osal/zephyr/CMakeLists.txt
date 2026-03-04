zephyr_library_sources(
  osal_thread.c
  osal_sync.c
  osal_time.c
  osal_socket.c
  osal_flash.c
  osal_atomic.c
  osal_net.c
)

# OTA-specific OSAL wrappers (only when OTA is enabled)
if(CONFIG_CEELL_OTA)
  zephyr_library_sources(
    osal_dfu.c
    osal_http.c
    osal_stream_flash.c
  )
endif()
