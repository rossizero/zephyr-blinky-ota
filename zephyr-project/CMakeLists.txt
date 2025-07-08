cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(esp32_blinky_ota)

target_sources(app PRIVATE
    src/main.c
    src/blinky.c
    #src/wifi_mgmt.c
    #src/ota_mgmt.c
)

target_include_directories(app PRIVATE include)