# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sample_with_mcuboot)

# Verify that this sample is built through sysbuild to ensure MCUboot is
# automatically included and that sample specific MCUboot configurations are
# used when building MCUboot.
test_sysbuild()

# Fügt Ihre C-Quelldateien zum "app"-Ziel hinzu.
target_sources(app PRIVATE
    src/main.c
    src/blinky.c
    src/wifi_mgmt.c
    src/ota_mgmt.c
)

#set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD}.overlay)

target_include_directories(app PRIVATE 
    include
)

if(CONFIG_RISCV AND CONFIG_SOC_SERIES_ESP32C3)
    target_compile_options(app PRIVATE -march=rv32ima)
    target_link_options(app PRIVATE -march=rv32ima)
endif()