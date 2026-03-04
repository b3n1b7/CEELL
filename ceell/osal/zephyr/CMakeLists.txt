target_sources(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/osal_thread.c
  ${CMAKE_CURRENT_SOURCE_DIR}/osal_sync.c
  ${CMAKE_CURRENT_SOURCE_DIR}/osal_time.c
  ${CMAKE_CURRENT_SOURCE_DIR}/osal_socket.c
  ${CMAKE_CURRENT_SOURCE_DIR}/osal_flash.c
  ${CMAKE_CURRENT_SOURCE_DIR}/osal_atomic.c
  ${CMAKE_CURRENT_SOURCE_DIR}/osal_net.c
)

# OTA-specific OSAL wrappers (only when OTA is enabled)
if(CONFIG_CEELL_OTA)
  target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/osal_dfu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/osal_http.c
    ${CMAKE_CURRENT_SOURCE_DIR}/osal_stream_flash.c
  )
endif()
