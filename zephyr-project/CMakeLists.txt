# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
#find_package(Sysbuild REQUIRED)
#project(sample_with_mcuboot)

# Verify that this sample is built through sysbuild to ensure MCUboot is
# automatically included and that sample specific MCUboot configurations are
# used when building MCUboot.
test_sysbuild()

project(zephyr_project)
add_subdirectory(app)
#add_subdirectory(sysbuild/mcuboot mcuboot)

