#ifndef OTA_MGMT_H
#define OTA_MGMT_H

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
    OTA_STATUS_SLEEPING,
} ota_status_t;

/* OTA error codes */
typedef enum {
    OTA_ERR_NONE = 0,
    OTA_ERR_SERVER_CONNECT,
    OTA_ERR_DOWNLOAD_FAILED,
    OTA_ERR_FLASH_INIT,
    OTA_ERR_FLASH_WRITE,
    OTA_ERR_APPLY_UPDATE,
    OTA_ERR_INVALID_IMAGE,
} ota_error_t;

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

#ifdef __cplusplus
}
#endif

#endif /* OTA_MGMT_H */