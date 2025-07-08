#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Application Configuration */
#define APP_NAME "ESP32_BLINKY_OTA"
#define APP_VERSION_MAJOR 1
#define APP_VERSION_MINOR 0
#define APP_VERSION_PATCH 0

/* LED Configuration */
#define LED_BLINK_INTERVAL_MS 1000
#define LED_FAST_BLINK_MS 100
#define LED_SLOW_BLINK_MS 2000

/* WiFi Configuration */
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PSK "YourWiFiPassword"

/* OTA Server Configuration */
#define OTA_SERVER_HOST "192.168.1.100"
#define OTA_SERVER_PORT 8080
#define OTA_VERSION_URL "/api/version"
#define OTA_FIRMWARE_URL "/api/firmware"

#endif /* APP_CONFIG_H */