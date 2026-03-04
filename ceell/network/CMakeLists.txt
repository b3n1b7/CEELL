target_sources(ceell PRIVATE
  discovery.c
  messaging.c
  msg_deadline.c
  service_discovery.c
)

if(CONFIG_CEELL_TSN)
  target_sources(ceell PRIVATE tsn.c)
endif()

if(CONFIG_CEELL_MSG_REDUNDANT)
  target_sources(ceell PRIVATE msg_redundancy.c)
endif()
