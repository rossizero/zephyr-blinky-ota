#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/stream_flash.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>
#include <string.h>
#include "wifi_mgmt.h"
#include "app_config.h"

LOG_MODULE_REGISTER(ota_mgmt, LOG_LEVEL_INF);

static struct k_work_delayable ota_check_work;
static struct stream_flash_ctx flash_ctx;
static uint8_t stream_buffer[1024]; /* Buffer für stream_flash */

/* Simple HTTP GET request */
static int http_get_simple(const char *host, int port, const char *path, 
                          char *response, size_t response_size)
{
    int sock;
    struct sockaddr_in addr;
    char request[512];
    int ret;
    
    sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) return -errno;
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    zsock_inet_pton(AF_INET, host, &addr.sin_addr);
    
    ret = zsock_connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        zsock_close(sock);
        return -errno;
    }
    
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
             path, host);
    
    zsock_send(sock, request, strlen(request), 0);
    ret = zsock_recv(sock, response, response_size - 1, 0);
    zsock_close(sock);
    
    if (ret > 0) {
        response[ret] = '\0';
    }
    
    return ret;
}

/* Check for firmware updates */
static bool check_for_update(void)
{
    char response[1024];
    int ret;
    
    ret = http_get_simple(OTA_SERVER_HOST, OTA_SERVER_PORT, 
                         OTA_VERSION_URL, response, sizeof(response));
    
    if (ret > 0) {
        /* Simple version check - look for newer version */
        if (strstr(response, "\"version\":\"1.0.1\"")) {
            LOG_INF("New firmware version available!");
            return true;
        }
    }
    
    return false;
}

/* Download and install firmware update */
static int perform_update(void)
{
    const struct flash_area *fa;
    int sock, ret;
    struct sockaddr_in addr;
    char request[512];
    uint8_t buffer[1024];
    size_t total_downloaded = 0;
    
    LOG_INF("Starting firmware download...");
    
    /* Initialize flash */
    ret = flash_area_open(1, &fa); /* Numerische ID für image_1 */
    if (ret) return ret;
    
    /* Korrekte stream_flash_init Parameter */
    ret = stream_flash_init(&flash_ctx, 
                           fa->fa_dev,              /* Flash device */
                           stream_buffer,           /* Buffer pointer */
                           sizeof(stream_buffer),   /* Buffer size */
                           fa->fa_off,              /* Flash offset */
                           fa->fa_size,             /* Flash size */
                           NULL);                   /* Callback function */
    
    if (ret) {
        flash_area_close(fa);
        return ret;
    }
    
    /* Download firmware */
    sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        flash_area_close(fa);
        return -errno;
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(OTA_SERVER_PORT);
    zsock_inet_pton(AF_INET, OTA_SERVER_HOST, &addr.sin_addr);
    
    ret = zsock_connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        zsock_close(sock);
        flash_area_close(fa);
        return -errno;
    }
    
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
             OTA_FIRMWARE_URL, OTA_SERVER_HOST);
    
    zsock_send(sock, request, strlen(request), 0);
    
    /* Skip HTTP headers */
    bool headers_done = false;
    char header_buf[4] = {0};
    int header_pos = 0;
    
    while (!headers_done) {
        char c;
        if (zsock_recv(sock, &c, 1, 0) <= 0) break;
        
        header_buf[header_pos] = c;
        if (++header_pos >= 4) {
            if (memcmp(header_buf, "\r\n\r\n", 4) == 0) {
                headers_done = true;
            }
            memmove(header_buf, header_buf + 1, 3);
            header_pos = 3;
        }
    }
    
    /* Download firmware data */
    while ((ret = zsock_recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        ret = stream_flash_buffered_write(&flash_ctx, buffer, ret, false);
        if (ret < 0) {
            LOG_ERR("Flash write failed: %d", ret);
            break;
        }
        total_downloaded += ret;
        
        if (total_downloaded % 4096 == 0) {
            LOG_INF("Downloaded: %zu bytes", total_downloaded);
        }
    }
    
    zsock_close(sock);
    flash_area_close(fa);
    
    if (ret >= 0) {
        /* Finalize flash write */
        ret = stream_flash_buffered_write(&flash_ctx, NULL, 0, true);
        
        if (ret == 0) {
            /* Mark for upgrade */
            ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
            if (ret == 0) {
                LOG_INF("Firmware update ready - rebooting in 3 seconds");
                k_msleep(3000);
                sys_reboot(SYS_REBOOT_WARM);
            }
        }
    }
    
    return ret;
}

/* OTA check work handler */
static void ota_check_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    
    if (wifi_is_connected()) {
        LOG_DBG("Checking for firmware updates...");
        
        if (check_for_update()) {
            perform_update();
        }
    }
    
    /* Schedule next check */
    k_work_schedule(&ota_check_work, K_SECONDS(60));
}

static int ota_mgmt_init(void)
{
    /* Initialize OTA check work */
    k_work_init_delayable(&ota_check_work, ota_check_work_handler);
    
    /* Start first check after 30 seconds */
    k_work_schedule(&ota_check_work, K_SECONDS(30));
    
    LOG_INF("OTA management subsystem initialized");
    return 0;
}

/* Auto-initialize at APPLICATION level */
SYS_INIT(ota_mgmt_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);