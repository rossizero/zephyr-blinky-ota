cmake_minimum_required(VERSION 3.20.0)

#set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../bootloader/mcuboot/boot/zephyr)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mcuboot)

# 1. Define where the REAL source code is
#set(ZEPHYR_MCUBOOT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../bootloader/mcuboot)

# 2. Execute the standard build script from that source location
#include(${ZEPHYR_MCUBOOT_SOURCE_DIR}/boot/zephyr/CMakeLists.txt)

#project(mcuboot)
