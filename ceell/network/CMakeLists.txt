zephyr_library()

zephyr_library_sources(
  discovery.c
  messaging.c
  msg_deadline.c
  service_discovery.c
)

if(CONFIG_CEELL_TSN)
  zephyr_library_sources(tsn.c)
endif()

if(CONFIG_CEELL_MSG_REDUNDANT)
  zephyr_library_sources(msg_redundancy.c)
endif()
