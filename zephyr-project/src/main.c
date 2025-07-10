#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/mcuboot.h>

#include "app_config.h"
#include "blinky.h"
#include "wifi_mgmt.h"
#include "ota_mgmt.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* OTA status callback */
static void ota_status_changed(ota_status_t status)
{
    switch (status) {
    case OTA_STATUS_CHECKING:
        LOG_INF("OTA: Checking for updates...");
        break;
    case OTA_STATUS_UPDATE_AVAILABLE:
        LOG_INF("OTA: Update available, preparing download");
        break;
    case OTA_STATUS_DOWNLOADING:
        LOG_INF("OTA: Downloading firmware update");
        break;
    case OTA_STATUS_DOWNLOAD_COMPLETE:
        LOG_INF("OTA: Download complete, preparing to apply");
        break;
    case OTA_STATUS_APPLYING:
        LOG_INF("OTA: Applying update, device will reboot");
        break;
    case OTA_STATUS_ERROR:
        LOG_ERR("OTA: Error occurred, code: %d", ota_get_last_error());
        break;
    default:
        break;
    }
}

int main(void)
{
    char version[16];
    ota_get_current_version(version, sizeof(version));
    
    LOG_INF("Starting ESP32 Blinky OTA v%s", version);
    
    /* Register OTA status callback */
    ota_register_status_callback(ota_status_changed);

    LOG_INF("Check if new firmware");
    /* If this is a new firmware, confirm it after 30 seconds */
    if (boot_is_img_confirmed() == 0) {
        LOG_INF("Running new firmware - will confirm after 30 seconds if stable");
        k_sleep(K_SECONDS(30));
        LOG_INF("Confirming running firmware as valid");
        ota_confirm_image();
    }
    
    LOG_INF("Main loop");
    while (1) {
        k_sleep(K_SECONDS(5));
        
        char ip_str[16];
        int result = wifi_get_ip_address_public(ip_str, sizeof(ip_str));
        if (result == 0) {
            LOG_INF("IP: %s", ip_str);
        } else {
            LOG_INF("No IP");
        }
    }
    
    return 0;
}