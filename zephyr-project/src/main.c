#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_config.h"
#include "blinky.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF); 

int main(void)
{
    LOG_INF("Starting ESP32 Blinky OTA v%d.%d.%d", 
        APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);

    /* Alle Subsysteme initialisieren sich automatisch über SYS_INIT */
    
    /* Optional: Status-Updates von anderen Subsystemen empfangen */
    while (1) {
        k_sleep(K_SECONDS(10));
        
        // WiFi-Status abfragen und LED entsprechend setzen
        // if (wifi_is_connected()) {
        //     led_set_status(LED_STATUS_WIFI_CONNECTED);
        // } else {
        //     led_set_status(LED_STATUS_WIFI_CONNECTING);
        // }
    }

    return 0;
}