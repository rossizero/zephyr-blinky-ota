#include "app_config.h"
#include "blinky.h"
#include "wifi_mgmt.h"
#include "ota_mgmt.h"
#include "utils.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/linker/linker-defs.h>


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
        ota_check_for_update();
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
            blinky_set_interval(LED_FAST_BLINK_MS);
            LOG_INF("OTA: Downloading firmware update");
            break;
        case OTA_STATUS_DOWNLOAD_COMPLETE:
            LOG_INF("OTA: Download complete, preparing to apply");
            break;
        case OTA_STATUS_APPLYING:
            LOG_INF("OTA: Applying update, device will reboot");
            break;
        case OTA_STATUS_ERROR:
            blinky_set_interval(LED_ERROR_PATTERN);
            LOG_ERR("OTA: Error occurred, code: %d", ota_get_last_error());
            break;
        case OTA_STATUS_SLEEPING:
            blinky_set_interval(LED_SLOW_BLINK_MS);
            LOG_INF("OTA: Going to sleep");
            break;
        default:
            blinky_set_interval(LED_BLINK_INTERVAL_MS);
            break;
    }
}

int main(void)
{
    LOG_INF("====================================");
    LOG_INF("   ESP32 OTA Application Booting    ");
    LOG_INF("====================================");

    if (boot_is_img_confirmed() == 0) {
        LOG_WRN("Running new firmware in TEST mode.");
        LOG_WRN("Scheduling confirmation in 30 seconds.");
        k_work_schedule(&confirm_work, K_SECONDS(30));
    } else {
        LOG_INF("Running a confirmed image.");
    }
    LOG_DBG("Address of app %p\n", (void *)__rom_region_start);

    ota_register_status_callback(ota_status_changed);

    char current_ver[16];
    int rc = ota_get_running_firmware_version(current_ver, sizeof(current_ver));
    if (rc == 0) {
        LOG_INF("Current running version: %s", current_ver);
    } else {
        LOG_WRN("Something is wrong with the version getter");
    }
    
    bool ip_printed = false;

    LOG_INF("Main loop started. System is running.");
    while (1) {
        k_sleep(K_SECONDS(5));

        if(!ip_printed) {
            char ip_str[16];
            int result = wifi_get_ip_address_public(ip_str, sizeof(ip_str));
            if (result == 0) {
                LOG_INF("IP: %s", ip_str);
                ip_printed = true;
            } else {
                LOG_INF("No IP yet");
            }
        }
    }
    
    return 0;
}