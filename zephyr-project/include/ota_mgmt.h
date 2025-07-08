#ifndef OTA_MGMT_H
#define OTA_MGMT_H

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize OTA management subsystem
 * 
 * This function is called automatically during system initialization.
 * 
 * @return 0 on success, negative error code on failure
 */
int ota_mgmt_init(void);

#ifdef __cplusplus
}
#endif

#endif /* OTA_MGMT_H */