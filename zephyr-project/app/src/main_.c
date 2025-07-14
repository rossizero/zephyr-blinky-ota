#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("Starting Zephyr App with MCUboot support");
    
    // Image-Status überprüfen
    if (boot_is_img_confirmed()) {
        LOG_INF("Image is confirmed");
    } else {
        LOG_INF("Image is not confirmed, confirming now");
        boot_write_img_confirmed();
    }
    
    // Haupt-App-Logik hier
    while (1) {
        LOG_INF("App running...");
        k_sleep(K_SECONDS(5));
    }
    
    return 0;
}