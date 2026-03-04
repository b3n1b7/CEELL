target_sources(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/discovery.c
  ${CMAKE_CURRENT_SOURCE_DIR}/messaging.c
  ${CMAKE_CURRENT_SOURCE_DIR}/msg_deadline.c
  ${CMAKE_CURRENT_SOURCE_DIR}/service_discovery.c
)

if(CONFIG_CEELL_TSN)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tsn.c)
endif()

if(CONFIG_CEELL_MSG_REDUNDANT)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/msg_redundancy.c)
endif()
