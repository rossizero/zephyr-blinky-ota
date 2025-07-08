#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_config.h"
#include "blinky.h"
#include "wifi_mgmt.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("Starting ESP32 Blinky v%d.%d.%d", 
            APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);

    while (1) {
        k_sleep(K_SECONDS(5));  // 5 statt 2 Sekunden
        
        char ip_str[16];
        int result = wifi_get_ip_address(ip_str, sizeof(ip_str));
        LOG_INF("IP result: %d", result);
        if (result == 0) {
            LOG_INF("IP: %s", ip_str);
        }
    }

    return 0;
}