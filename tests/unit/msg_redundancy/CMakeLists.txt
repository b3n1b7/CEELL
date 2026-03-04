cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_msg_redundancy)

target_sources(app PRIVATE
  src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/network/msg_redundancy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal/zephyr/osal_sync.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal/zephyr/osal_socket.c
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/core
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/network
)

# Provide Kconfig values needed by msg_redundancy.c when built outside
# the main project build (the test has its own Kconfig/prj.conf scope).
target_compile_definitions(app PRIVATE
  CONFIG_CEELL_MSG_PORT_SECONDARY=5557
)
