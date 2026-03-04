target_sources(ceell PRIVATE
  config_parser.c
  net_setup.c
  node_identity.c
  lifecycle.c
  service_registry.c
  health_monitor.c
  safe_state.c
)

if(CONFIG_CEELL_TIMING)
  target_sources(ceell PRIVATE timing.c)
endif()

if(CONFIG_CEELL_TIMING_BUDGETS)
  target_sources(ceell PRIVATE timing_budget.c)
endif()

if(CONFIG_CEELL_WATCHDOG)
  target_sources(ceell PRIVATE watchdog.c)
endif()
