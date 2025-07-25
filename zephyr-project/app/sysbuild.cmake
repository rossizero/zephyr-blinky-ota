# This file is automatically included by the sysbuild system.
# It allows us to use CMake logic to control the build.

# Check if the board being built is the esp32s3_devkitc
# theoretically we could differ between boards here, to switch signature types
# but for some reason anything else but CONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256 leads to iram overflow
if(BOARD STREQUAL "esp32s3_devkitc____")
  # If it is, tell sysbuild to use our S3-specific config file.
  set(SB_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/sysbuild-esp32s3.conf)
else()
  # For any other board, use the default config file.
  set(SB_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/sysbuild-default.conf)
endif()

# Print the chosen file for debugging purposes.
message(STATUS "Sysbuild: Using configuration file: ${SB_CONF_FILE}")