cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_health_monitor)

target_sources(app PRIVATE
  src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/core/health_monitor.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal/zephyr/osal_sync.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal/zephyr/osal_time.c
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/core
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/network
)
