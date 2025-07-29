#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Debug function to print image headers from both slots
 * 
 * This function reads and logs the MCUboot image headers from both
 * slot0 (currently running) and slot1 (OTA update) partitions.
 * Useful for debugging OTA update status and image information.
 */
void debug_image_headers(void);

/**
 * @brief Get the version of the currently running firmware.
 *
 * This function reads the image header from the primary (active) slot,
 * formats the semantic version into a string, and stores it in the
 * provided buffer.
 *
 * @param buf       A character buffer to store the version string.
 * @param buf_size  The size of the provided buffer.
 *
 * @return 0 on success.
 * @return -EIO if the image header could not be read.
 * @return -ENOMEM if the provided buffer is too small to hold the version string.
 * @return A negative error code from the underlying boot/flash functions on other failures.
 */
int ota_get_running_firmware_version(char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */