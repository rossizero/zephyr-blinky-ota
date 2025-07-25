#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/stream_flash.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/data/json.h>
#include <zephyr/drivers/watchdog.h>
#include <string.h>
#include "wifi_mgmt.h"
#include "app_config.h"
#include "ota_mgmt.h"
#include "blinky.h"
#include <zephyr/devicetree.h>


LOG_MODULE_REGISTER(ota_mgmt, LOG_LEVEL_INF);

#define FLASH_AREA_IMAGE_SECONDARY DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition))


/* OTA state variables */
static struct k_work_delayable ota_check_work;
static struct stream_flash_ctx flash_ctx;
static uint8_t stream_buffer[1024]; /* Buffer for stream_flash */
static ota_status_t current_status = OTA_STATUS_IDLE;
static ota_error_t last_error = OTA_ERR_NONE;
static void (*status_callback)(ota_status_t) = NULL;

/* HTTP client context */
static struct http_request http_req;
static uint8_t http_recv_buf[1024];
static int64_t content_length = 0;
static size_t total_downloaded = 0;
static bool headers_complete = false;
static int retry_count = 0;
static int http_sock = -1;

/* Watchdog */
static const struct device *wdt;
static int wdt_channel_id = -1;

/* Version parsing from JSON */
struct version_info {
    const char *version;
    int size;
};

static struct json_obj_descr version_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct version_info, version, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct version_info, size, JSON_TOK_NUMBER),
};

/* Forward declarations */
static void ota_check_work_handler(struct k_work *work);
static int perform_update(void);
static void update_status(ota_status_t new_status);
static int apply_update(void);
static void setup_watchdog(void);
static void feed_watchdog(void);
static int create_http_socket(const char *host, int port);

/* Update OTA status and trigger callback if registered */
static void update_status(ota_status_t new_status)
{
    if (current_status != new_status) {
        current_status = new_status;
        
        /* Update LED pattern based on status */
        switch (new_status) {
        case OTA_STATUS_DOWNLOADING:
            blinky_set_interval(LED_SLOW_BLINK_MS);
            break;
        case OTA_STATUS_APPLYING:
            blinky_set_interval(LED_FAST_BLINK_MS);
            break;
        case OTA_STATUS_ERROR:
            blinky_set_interval(LED_ERROR_PATTERN);
            break;
        default:
            blinky_set_interval(LED_BLINK_INTERVAL_MS);
            break;
        }
        
        if (status_callback != NULL) {
            status_callback(new_status);
        }
    }
}

/* Set error and update status */
static void set_error(ota_error_t error)
{
    last_error = error;
    update_status(OTA_STATUS_ERROR);
}

/* Setup watchdog timer */
static void setup_watchdog(void)
{
    wdt = DEVICE_DT_GET_OR_NULL(DT_ALIAS(wdt0));
    if (!wdt || !device_is_ready(wdt)) {
        LOG_ERR("Watchdog device not found or not ready");
        return;
    }
    
    struct wdt_timeout_cfg wdt_config = {
        .window.min = 0,
        .window.max = 10000,  // 10 seconds
        .callback = NULL,
        .flags = WDT_FLAG_RESET_SOC,
    };
    
    wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
    if (wdt_channel_id < 0) {
        LOG_ERR("Watchdog install error: %d", wdt_channel_id);
        return;
    }
    
    int ret = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
    if (ret < 0) {
        LOG_ERR("Watchdog setup error: %d", ret);
        return;
    }
    
    LOG_INF("Watchdog initialized with timeout of 10 seconds");
}

/* Feed the watchdog */
static void feed_watchdog(void)
{
    if (wdt && wdt_channel_id >= 0) {
        wdt_feed(wdt, wdt_channel_id);
    }
}

/* Create HTTP socket connection */
static int create_http_socket(const char *host, int port)
{
    struct zsock_addrinfo hints, *result;
    struct sockaddr_in addr;
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
    
    /* Connect */
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

/* HTTP response callback */
static int http_response_cb(struct http_response *rsp, 
                           enum http_final_call final_data,
                           void *user_data)
{
    int ret = 0;
    
    /* Feed watchdog to prevent reset during download */
    feed_watchdog();
    
    /* Check if this is the first time we get body data */
    if (!headers_complete && rsp->body_frag_len > 0) {
        headers_complete = true;
        content_length = rsp->content_length;
        LOG_INF("HTTP headers complete, content length: %lld", content_length);
        
        /* If checking version, parse the JSON response */
        if (current_status == OTA_STATUS_CHECKING) {
            struct version_info version = {0};
            
            ret = json_obj_parse(rsp->body_frag_start, rsp->body_frag_len,
                               version_descr, ARRAY_SIZE(version_descr),
                               &version);
            
            if (ret > 0 && version.version) {
                LOG_INF("Server version: %s", version.version);
                
                /* Compare with current version */
                char current_ver[16];
                snprintf(current_ver, sizeof(current_ver), "%d.%d.%d", 
                        APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);
                
                if (strcmp(version.version, current_ver) != 0) {
                    LOG_INF("New version available: %s (current: %s)",
                           version.version, current_ver);
                    update_status(OTA_STATUS_UPDATE_AVAILABLE);
                    
                    /* Start download after a short delay */
                    k_work_schedule(&ota_check_work, K_SECONDS(10));
                } else {
                    LOG_INF("Already running latest version");
                    update_status(OTA_STATUS_IDLE);
                }
            } else {
                LOG_ERR("Failed to parse version info");
                set_error(OTA_ERR_SERVER_CONNECT);
            }
            
            return 0;
        }
        
        /* If downloading firmware, initialize flash */
        if (current_status == OTA_STATUS_DOWNLOADING) {
            const struct flash_area *fa;
            
            ret = flash_area_open(FLASH_AREA_IMAGE_SECONDARY, &fa);
            if (ret) {
                LOG_ERR("Failed to open flash area: %d", ret);
                set_error(OTA_ERR_FLASH_INIT);
                return ret;
            }
            
            /* Erase the flash area before writing */
            ret = flash_area_erase(fa, 0, fa->fa_size);
            if (ret) {
                LOG_ERR("Failed to erase flash area: %d", ret);
                flash_area_close(fa);
                set_error(OTA_ERR_FLASH_INIT);
                return ret;
            }
            
            ret = stream_flash_init(&flash_ctx, fa->fa_dev, stream_buffer,
                                  sizeof(stream_buffer), fa->fa_off,
                                  fa->fa_size, NULL);
            
            if (ret) {
                LOG_ERR("Failed to init stream flash: %d", ret);
                flash_area_close(fa);
                set_error(OTA_ERR_FLASH_INIT);
                return ret;
            }
            
            LOG_INF("Flash initialized for firmware download");
            total_downloaded = 0;
        }
    }
    
    /* Process body data for firmware download */
    if (headers_complete && current_status == OTA_STATUS_DOWNLOADING && 
        rsp->body_frag_len > 0) {
        
        ret = stream_flash_buffered_write(&flash_ctx, 
                                        rsp->body_frag_start,
                                        rsp->body_frag_len, 
                                        final_data == HTTP_DATA_FINAL);
        
        if (ret < 0) {
            LOG_ERR("Flash write error: %d", ret);
            set_error(OTA_ERR_FLASH_WRITE);
            return ret;
        }
        
        total_downloaded += rsp->body_frag_len;
        
        if (total_downloaded % 4096 == 0 || final_data == HTTP_DATA_FINAL) {
            double progress = (double)total_downloaded * 100.0 / content_length;
            LOG_INF("Downloaded: %zu bytes (%.1f%%)", total_downloaded, progress);
        }
        
        /* Check if download is complete */
        if (final_data == HTTP_DATA_FINAL) {
            LOG_INF("Firmware download complete: %zu bytes", total_downloaded);
            update_status(OTA_STATUS_DOWNLOAD_COMPLETE);
            
            /* Apply update after a short delay */
            k_work_schedule(&ota_check_work, K_SECONDS(1));
        }
    }
    
    return 0;
}

/* Check for firmware updates */
static int check_for_update(void)
{
    if (!wifi_is_connected()) {
        LOG_WRN("WiFi not connected, skipping update check");
        return -ENOTCONN;
    }
    
    update_status(OTA_STATUS_CHECKING);
    
    /* Create socket connection */
    http_sock = create_http_socket(OTA_SERVER_HOST, OTA_SERVER_PORT);
    if (http_sock < 0) {
        set_error(OTA_ERR_SERVER_CONNECT);
        return -ECONNREFUSED;
    }
    
    /* Setup HTTP request for version check */
    memset(&http_req, 0, sizeof(http_req));
    
    http_req.method = HTTP_GET;
    http_req.url = OTA_VERSION_URL;
    http_req.host = OTA_SERVER_HOST;
    http_req.protocol = "HTTP/1.1";
    http_req.response = http_response_cb;
    http_req.recv_buf = http_recv_buf;
    http_req.recv_buf_len = sizeof(http_recv_buf);
    
    headers_complete = false;
    
    LOG_INF("Checking for updates at http://%s:%d%s", 
           OTA_SERVER_HOST, OTA_SERVER_PORT, OTA_VERSION_URL);
    
    int ret = http_client_req(http_sock, &http_req, 5000, NULL);
    
    /* Close socket */
    zsock_close(http_sock);
    http_sock = -1;
    
    if (ret < 0) {
        LOG_ERR("Failed to connect to update server: %d", ret);
        set_error(OTA_ERR_SERVER_CONNECT);
    }
    
    return ret;
}

/* Download and install firmware update */
static int perform_update(void)
{
    LOG_INF("Tryning to download firmware");
    if (!wifi_is_connected()) {
        LOG_WRN("WiFi not connected, cannot download update");
        return -ENOTCONN;
    }
    
    update_status(OTA_STATUS_DOWNLOADING);
    
    /* Create socket connection */
    http_sock = create_http_socket(OTA_SERVER_HOST, OTA_SERVER_PORT);
    if (http_sock < 0) {
        set_error(OTA_ERR_SERVER_CONNECT);
        return -ECONNREFUSED;
    }
    
    /* Setup HTTP request for firmware download */
    memset(&http_req, 0, sizeof(http_req));
    
    http_req.method = HTTP_GET;
    http_req.url = OTA_FIRMWARE_URL;
    http_req.host = OTA_SERVER_HOST;
    http_req.protocol = "HTTP/1.1";
    http_req.response = http_response_cb;
    http_req.recv_buf = http_recv_buf;
    http_req.recv_buf_len = sizeof(http_recv_buf);
    
    headers_complete = false;
    
    LOG_INF("Downloading firmware from http://%s:%d%s", 
           OTA_SERVER_HOST, OTA_SERVER_PORT, OTA_FIRMWARE_URL);
    
    int ret = http_client_req(http_sock, &http_req, OTA_DOWNLOAD_TIMEOUT_MS, NULL);
    
    /* Close socket */
    zsock_close(http_sock);

    http_sock = -1;
    
    if (ret < 0) {
        LOG_ERR("Failed to download firmware: %d", ret);
        set_error(OTA_ERR_DOWNLOAD_FAILED);
        
        /* Retry logic */
        if (++retry_count < OTA_MAX_DOWNLOAD_RETRIES) {
            LOG_INF("Retrying download (%d/%d) in 5 seconds...", 
                   retry_count + 1, OTA_MAX_DOWNLOAD_RETRIES);
            k_work_schedule(&ota_check_work, K_SECONDS(5));
        } else {
            LOG_ERR("Max retry attempts reached, giving up");
            retry_count = 0;
            update_status(OTA_STATUS_IDLE);
        }
    } else {
        /* Reset retry counter on success */
        retry_count = 0;
    }
    
    return ret;
}

/* Apply downloaded update */
static int apply_update(void)
{
    update_status(OTA_STATUS_APPLYING);
    
    /* Mark image for testing */
    int ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
    
    if (ret != 0) {
        LOG_ERR("Failed to request upgrade: %d", ret);
        set_error(OTA_ERR_APPLY_UPDATE);
        return ret;
    }
    
    LOG_INF("Update ready - rebooting in 3 seconds");
    k_sleep(K_SECONDS(3));
    sys_reboot(SYS_REBOOT_WARM);
    
    return 0; /* Will never reach here */
}

/* OTA check work handler */
static void ota_check_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    
    /* Feed watchdog */
    feed_watchdog();
    
    switch (current_status) {
    case OTA_STATUS_IDLE:
        check_for_update();
        break;
        
    case OTA_STATUS_UPDATE_AVAILABLE:
        perform_update();
        break;
        
    case OTA_STATUS_DOWNLOAD_COMPLETE:
        apply_update();
        break;
        
    default:
        break;
    }
}

/* Public functions */
int ota_check_for_update(void)
{
    if (current_status != OTA_STATUS_IDLE) {
        LOG_WRN("OTA operation already in progress");
        return -EBUSY;
    }
    
    return check_for_update();
}

ota_status_t ota_get_status(void)
{
    return current_status;
}

ota_error_t ota_get_last_error(void)
{
    return last_error;
}

void ota_register_status_callback(void (*callback)(ota_status_t status))
{
    status_callback = callback;
}

int ota_get_current_version(char *version, size_t len)
{
    if (!version || len < 8) {
        return -EINVAL;
    }
    
    snprintf(version, len, "%d.%d.%d", 
             APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);
    
    return 0;
}

int ota_confirm_image(void)
{
    return boot_write_img_confirmed();
}

int ota_mgmt_init(void)
{
    /* Initialize OTA check work */
    k_work_init_delayable(&ota_check_work, ota_check_work_handler);
    
    /* Setup watchdog */
    setup_watchdog();
    
    /* Check if we're running a test image */
    if (boot_is_img_confirmed() == 0) {
        LOG_INF("Running unconfirmed image - waiting for validation");
        
        /* Schedule image confirmation after 30 seconds if everything works */
        k_work_schedule(&ota_check_work, K_SECONDS(15));
    } else {
        /* Start first update check after 60 seconds */
        k_work_schedule(&ota_check_work, K_SECONDS(35));
    }
    
    LOG_INF("OTA management subsystem initialized");
    return 0;
}

/* Auto-initialize at APPLICATION level */
SYS_INIT(ota_mgmt_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);