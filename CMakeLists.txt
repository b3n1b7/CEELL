cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ceell VERSION 0.1.0)

add_subdirectory(ceell)

target_sources(app PRIVATE src/main.c)
