#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* LED Configuration */
#define LED_BLINK_INTERVAL_MS 1000
#define LED_SLOW_BLINK_MS 2000
#define LED_FAST_BLINK_MS 100
#define LED_ERROR_PATTERN 250

/* WiFi Configuration */
#define WIFI_SSID " :)"
#define WIFI_PSK "Schaezler17"

/* OTA Server Configuration */
#define OTA_SERVER_HOST "172.27.48.159" //"192.168.2.86"  // Replace with your server IP
#define OTA_SERVER_PORT 8080
#define OTA_VERSION_URL "/api/version"
#define OTA_FIRMWARE_URL "/api/firmware"

/* OTA Update Configuration */
#define OTA_CHECK_INTERVAL_SEC 3600  // Check for updates every hour
#define OTA_MAX_DOWNLOAD_RETRIES 3
#define OTA_DOWNLOAD_TIMEOUT_MS 30000

#endif /* APP_CONFIG_H */