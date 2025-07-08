#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_config.h"
#include "blinky.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("Starting ESP32 Blinky v%d.%d.%d", 
            APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);

    while (1) {
        k_sleep(K_SECONDS(2));
        //LOG_INF("LED mode: %d", led_get_mode());
    }

    return 0;
}