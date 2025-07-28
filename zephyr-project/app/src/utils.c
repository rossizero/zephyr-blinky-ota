#include "utils.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(utils, LOG_LEVEL_INF);

void debug_image_headers(void)
{
    LOG_INF("=== Image Header Debug ===");
    
    struct mcuboot_img_header header;
    int ret;
    
    /* Slot0 (aktuell laufend) prüfen */
    ret = boot_read_bank_header(DT_FIXED_PARTITION_ID(DT_NODELABEL(slot0_partition)), &header, sizeof(header));
    if (ret == 0) {
        LOG_INF("Slot0 - Image size: %u", header.h.v1.image_size);
        LOG_INF("Slot0 - Version: %u.%u.%u+%u", 
               header.h.v1.sem_ver.major,
               header.h.v1.sem_ver.minor,
               header.h.v1.sem_ver.revision,
               header.h.v1.sem_ver.build_num);
    } else {
        LOG_ERR("Failed to read slot0 header: %d", ret);
    }
    
    /* Slot1 (OTA Update) prüfen */
    ret = boot_read_bank_header(DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition)), &header, sizeof(header));
    if (ret == 0) {
        LOG_INF("✓ Slot1 - Valid image found!");
        LOG_INF("Slot1 - Image size: %u", header.h.v1.image_size);
        LOG_INF("Slot1 - Version: %u.%u.%u+%u", 
               header.h.v1.sem_ver.major,
               header.h.v1.sem_ver.minor,
               header.h.v1.sem_ver.revision,
               header.h.v1.sem_ver.build_num);
    } else {
        LOG_ERR("✗ Slot1 - No valid image found: %d", ret);
        
        /* Raw Flash-Daten zur Diagnose lesen */
        const struct flash_area *fa;
        if (flash_area_open(DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition)), &fa) == 0) {
            uint32_t magic;
            flash_area_read(fa, 0, &magic, sizeof(magic));
            LOG_ERR("Slot1 raw magic: 0x%08x (expected: 0x96f3b83d)", magic);
            flash_area_close(fa);
        }
    }
}

int ota_get_running_firmware_version(char *buf, size_t buf_size)
{
    struct mcuboot_img_header header;
    int rc;

    /*
     * Use boot_fetch_active_slot() to reliably get the ID of the slot
     * from which the current application was booted. This is more robust
     * than hardcoding the slot0 ID, especially in revert scenarios.
     */
    // TODO: not working on esp32_s3 boot_fetch_active_slot 
    uint8_t active_slot_id = boot_fetch_active_slot(); 

    rc = boot_read_bank_header(DT_FIXED_PARTITION_ID(DT_NODELABEL(slot0_partition)), &header, sizeof(header));
    if (rc != 0) {
        LOG_ERR("Failed to read running image header from slot %u. Error: %d",
                active_slot_id, rc);
        return rc; // Propagate the error (-EIO is common for empty/bad slots)
    }

    /* Format the version string into the provided buffer */
    rc = snprintf(buf, buf_size, "%u.%u.%u",
                  header.h.v1.sem_ver.major,
                  header.h.v1.sem_ver.minor,
                  header.h.v1.sem_ver.revision);

    /*
     * snprintf returns the number of characters that *would have been* written.
     * We must check if this number is greater than or equal to the buffer size,
     * which indicates truncation.
     */
    if (rc < 0 || rc >= buf_size) {
        LOG_ERR("Buffer too small for version string. Required: %d, available: %zu",
                rc, buf_size);
        return -ENOMEM; // No memory/space available
    }

    return 0; // Success
}