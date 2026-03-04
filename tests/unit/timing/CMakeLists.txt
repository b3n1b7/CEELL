cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_timing)

target_sources(app PRIVATE
  src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/core/timing.c
)

# OSAL time implementation (ceell_cycle_get, ceell_uptime_get, etc.)
target_sources(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal/zephyr/osal_time.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal/zephyr/osal_sync.c
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/core
)
