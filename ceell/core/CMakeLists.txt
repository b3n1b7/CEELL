zephyr_library_sources(
  config_parser.c
  net_setup.c
  node_identity.c
  lifecycle.c
  service_registry.c
  health_monitor.c
  safe_state.c
)

if(CONFIG_CEELL_TIMING)
  zephyr_library_sources(timing.c)
endif()

if(CONFIG_CEELL_TIMING_BUDGETS)
  zephyr_library_sources(timing_budget.c)
endif()

if(CONFIG_CEELL_WATCHDOG)
  zephyr_library_sources(watchdog.c)
endif()
