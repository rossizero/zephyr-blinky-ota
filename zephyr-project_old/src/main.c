#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/mcuboot.h>

#include "app_config.h"
#include "blinky.h"
#include "wifi_mgmt.h"
#include "ota_mgmt.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

// Define a work item for the delayed confirmation
static void confirm_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(confirm_work, confirm_work_handler);

static void confirm_work_handler(struct k_work *work)
{
    LOG_INF("Confirming running firmware as valid.");
    if (boot_write_img_confirmed() != 0) {
        LOG_ERR("Failed to confirm image! This may cause a revert on next boot.");
    } else {
        LOG_INF("Image confirmed successfully. The update is now permanent.");
    }
}

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
    
    LOG_INF("====================================");
    LOG_INF("   ESP32 OTA Application Booting    ");
    LOG_INF("====================================");

    // This is the most important part after an OTA update.
    // Check if we are running an unconfirmed image.
    if (boot_is_img_confirmed() == 0) {
        LOG_WRN("Running new firmware in TEST mode.");
        LOG_WRN("Scheduling confirmation in 30 seconds.");
        // We wait 30 seconds to ensure the system is stable (e.g., WiFi connects)
        // before making the update permanent.
        k_work_schedule(&confirm_work, K_SECONDS(30));
    } else {
        LOG_INF("Running a confirmed image.");
    }

    // Initialize subsystems
    wifi_init(); // Your function to start WiFi
    ota_register_status_callback(ota_status_changed);
    // ota_mgmt_init() is called automatically via SYS_INIT

    ota_get_current_version(version, sizeof(version));
    LOG_INF("Firmware Version: %s", version);

    LOG_INF("Main loop started. System is running.");
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