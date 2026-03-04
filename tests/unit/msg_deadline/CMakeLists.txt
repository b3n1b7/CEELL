cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_msg_deadline)

target_sources(app PRIVATE
  src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/network/msg_deadline.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal/zephyr/osal_time.c
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/network
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal
)
