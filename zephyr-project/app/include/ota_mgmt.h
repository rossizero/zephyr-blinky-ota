#ifndef OTA_MGMT_H
#define OTA_MGMT_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/dfu/mcuboot.h>

#ifdef __cplusplus
extern "C" {
#endif

/* OTA update status codes */
typedef enum {
    OTA_STATUS_IDLE = 0,
    OTA_STATUS_CHECKING,
    OTA_STATUS_UPDATE_AVAILABLE,
    OTA_STATUS_DOWNLOADING,
    OTA_STATUS_DOWNLOAD_COMPLETE,
    OTA_STATUS_APPLYING,
    OTA_STATUS_ERROR,
} ota_status_t;

/* OTA error codes */
typedef enum {
    OTA_ERR_NONE = 0,
    OTA_ERR_SERVER_CONNECT,
    OTA_ERR_DOWNLOAD_FAILED,
    OTA_ERR_FLASH_INIT,
    OTA_ERR_FLASH_WRITE,
    OTA_ERR_VERIFICATION,
    OTA_ERR_APPLY_UPDATE,
    OTA_ERR_INVALID_IMAGE,
} ota_error_t;

/**
 * @brief Initialize OTA management subsystem
 * 
 * This function is called automatically during system initialization.
 * 
 * @return 0 on success, negative error code on failure
 */
int ota_mgmt_init(void);

/**
 * @brief Manually trigger an OTA update check
 * 
 * @return 0 if check was started, negative error code on failure
 */
int ota_check_for_update(void);

/**
 * @brief Get current OTA status
 * 
 * @return Current OTA status code
 */
ota_status_t ota_get_status(void);

/**
 * @brief Get last OTA error
 * 
 * @return Last error code
 */
ota_error_t ota_get_last_error(void);

/**
 * @brief Register callback for OTA status changes
 * 
 * @param callback Function to call when OTA status changes
 */
void ota_register_status_callback(void (*callback)(ota_status_t status));

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
static void ota_enter_backoff_state(void);

#ifdef __cplusplus
}
#endif

#endif /* OTA_MGMT_H */