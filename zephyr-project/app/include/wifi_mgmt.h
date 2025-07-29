#ifndef WIFI_MGMT_H
#define WIFI_MGMT_H

#include <stdbool.h>
#include <stddef.h>

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
 * @brief Get current WiFi IP address as string
 * @param[out] ip_str Buffer for IP address string (min. 16 bytes)
 * @param[in] len Buffer size
 * @return 0 on success, negative error code on failure
 */
int wifi_get_ip_address_public(char *ip_str, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MGMT_H */