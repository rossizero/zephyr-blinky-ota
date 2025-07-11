cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(esp32_blinky_ota)

set(mcuboot_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/child_image/mcuboot/prj.conf)

target_sources(app PRIVATE
    src/main.c
    src/blinky.c
    src/wifi_mgmt.c
    src/ota_mgmt.c
)

target_include_directories(app PRIVATE include)