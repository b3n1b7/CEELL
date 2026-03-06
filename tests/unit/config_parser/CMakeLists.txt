cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_config_parser)

target_sources(app PRIVATE
  src/main.c
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/osal
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../ceell/core
)

target_compile_definitions(app PRIVATE
  CONFIG_CEELL_NODE_ID_DEFAULT=0
  CONFIG_CEELL_NODE_ROLE_DEFAULT=\"unknown\"
  CONFIG_CEELL_NODE_NAME_DEFAULT=\"ceell-unprovisioned\"
  CONFIG_CEELL_NODE_IP_DEFAULT=\"192.168.1.100\"
  CONFIG_CEELL_NODE_NETMASK_DEFAULT=\"255.255.255.0\"
)
