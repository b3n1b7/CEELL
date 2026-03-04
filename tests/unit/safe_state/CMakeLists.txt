cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_safe_state)

target_sources(app PRIVATE
  src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/core/safe_state.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal/zephyr/osal_sync.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal/zephyr/osal_atomic.c
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/core
)
