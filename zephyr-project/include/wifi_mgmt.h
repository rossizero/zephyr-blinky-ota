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
 * @brief Get IP address as string
 * 
 * @param ip_str Buffer to store IP address
 * @param len Buffer length
 * @return 0 on success, -1 on error
 */
int wifi_get_ip_address(char *ip_str, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MGMT_H */