#ifndef WIFI_MGMT_H
#define WIFI_MGMT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if WiFi is connected
 * 
 * @return true if WiFi is connected, false otherwise
 */
bool wifi_is_connected(void);

/**
 * @brief Initialize WiFi management subsystem
 * 
 * This function is called automatically during system initialization.
 * 
 * @return 0 on success, negative error code on failure
 */
int wifi_mgmt_init(void);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MGMT_H */