#include "utils.h"

#include <stddef.h>                     // size_t
#include <stdio.h>                      // snprintf
#include <zephyr/devicetree.h>          // DT_FIXED_PARTITION_ID, DT_NODELABEL
#include <zephyr/dfu/mcuboot.h>         // mcuboot_img_header, boot_* functions
#include <zephyr/logging/log.h>         // LOG_* macros
#include <zephyr/storage/flash_map.h>   // flash_area_* functions


LOG_MODULE_REGISTER(utils, LOG_LEVEL_INF);

void debug_image_headers(void)
{
    //please ignore the duplicate code for slot0 and slot1 :)
    LOG_DBG("=== Image Header Debug ===");
    
    struct mcuboot_img_header header;
    int ret;
    
    // check Slot0
    ret = boot_read_bank_header(DT_FIXED_PARTITION_ID(DT_NODELABEL(slot0_partition)), &header, sizeof(header));
    if (ret == 0) {
        LOG_DBG("Slot0 - Image size: %u", header.h.v1.image_size);
        LOG_DBG("Slot0 - Version: %u.%u.%u+%u", 
               header.h.v1.sem_ver.major,
               header.h.v1.sem_ver.minor,
               header.h.v1.sem_ver.revision,
               header.h.v1.sem_ver.build_num);
    } else {
        LOG_DBG("Failed to read slot0 header: %d", ret);
        const struct flash_area *fa;
        if (flash_area_open(DT_FIXED_PARTITION_ID(DT_NODELABEL(slot0_partition)), &fa) == 0) {
            uint32_t magic;
            flash_area_read(fa, 0, &magic, sizeof(magic));
            LOG_DBG("Slot0 raw magic: 0x%08x", magic);
            flash_area_close(fa);
        }
    }
    
    // check Slot1
    ret = boot_read_bank_header(DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition)), &header, sizeof(header));
    if (ret == 0) {
        LOG_DBG("Slot1 - Image size: %u", header.h.v1.image_size);
        LOG_DBG("Slot1 - Version: %u.%u.%u+%u", 
               header.h.v1.sem_ver.major,
               header.h.v1.sem_ver.minor,
               header.h.v1.sem_ver.revision,
               header.h.v1.sem_ver.build_num);
    } else {
        LOG_DBG("Failed to read slot1 header: %d", ret);
        const struct flash_area *fa;
        if (flash_area_open(DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition)), &fa) == 0) {
            uint32_t magic;
            flash_area_read(fa, 0, &magic, sizeof(magic));
            LOG_DBG("Slot1 raw magic: 0x%08x", magic);
            flash_area_close(fa);
        }
    }
}

int ota_get_running_firmware_version(char *buf, size_t buf_size)
{
    struct mcuboot_img_header header;
    int rc;

    // TODO: boot_fetch_active_slot() not working on esp32_s3 boot_fetch_active_slot 
    uint8_t active_slot_id = boot_fetch_active_slot(); 
    uint8_t slot_id = DT_FIXED_PARTITION_ID(DT_NODELABEL(slot0_partition));
    LOG_INF("Active Slot ID: %u, hard coded Slot ID: %u", active_slot_id, slot_id);
    rc = boot_read_bank_header(slot_id, &header, sizeof(header));
    if (rc != 0) {
        LOG_ERR("Failed to read running image header from slot %u. Error: %d",
                active_slot_id, rc);
        return rc;
    }
    rc = snprintf(buf, buf_size, "%u.%u.%u",
                  header.h.v1.sem_ver.major,
                  header.h.v1.sem_ver.minor,
                  header.h.v1.sem_ver.revision);

    if (rc < 0 || rc >= buf_size) {
        LOG_ERR("Buffer too small for version string. Required: %d, available: %zu",
                rc, buf_size);
        return -ENOMEM;
    }

    return 0;
}

int create_http_socket(const char *host, int port)
{
    struct zsock_addrinfo hints, *result;
    int sock;
    int ret;
    
    sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        LOG_ERR("Failed to create socket: %d", errno);
        return -1;
    }
    
    /* Set socket timeout */
    struct zsock_timeval timeout = {
        .tv_sec = 30,
        .tv_usec = 0,
    };
    
    zsock_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    zsock_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    /* Setup address resolution */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    ret = zsock_getaddrinfo(host, port_str, &hints, &result);
    if (ret != 0) {
        LOG_ERR("Failed to resolve hostname %s: %d", host, ret);
        zsock_close(sock);
        return -1;
    }
    
    ret = zsock_connect(sock, result->ai_addr, result->ai_addrlen);
    zsock_freeaddrinfo(result);
    
    if (ret < 0) {
        LOG_ERR("Failed to connect to %s:%d: %d", host, port, errno);
        zsock_close(sock);
        return -1;
    }
    
    LOG_INF("Connected to %s:%d", host, port);
    return sock;
}