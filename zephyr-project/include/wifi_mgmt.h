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

int wifi_get_ip_address_public(char *ip_str, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* WIFI_MGMT_H */