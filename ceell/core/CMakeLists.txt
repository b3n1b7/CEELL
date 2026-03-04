target_sources(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/config_parser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/net_setup.c
  ${CMAKE_CURRENT_SOURCE_DIR}/node_identity.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lifecycle.c
  ${CMAKE_CURRENT_SOURCE_DIR}/service_registry.c
  ${CMAKE_CURRENT_SOURCE_DIR}/health_monitor.c
  ${CMAKE_CURRENT_SOURCE_DIR}/safe_state.c
)

if(CONFIG_CEELL_TIMING)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/timing.c)
endif()

if(CONFIG_CEELL_TIMING_BUDGETS)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/timing_budget.c)
endif()

if(CONFIG_CEELL_WATCHDOG)
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/watchdog.c)
endif()
