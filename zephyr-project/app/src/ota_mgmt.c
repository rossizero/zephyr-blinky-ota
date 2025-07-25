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
#include <string.h>
#include "wifi_mgmt.h"
#include "app_config.h"
#include "ota_mgmt.h"
#include "blinky.h"
#include <zephyr/devicetree.h>
#include <zephyr/dfu/flash_img.h>
#include <stdio.h>


LOG_MODULE_REGISTER(ota_mgmt, LOG_LEVEL_INF);

struct flash_img_context image_ctx;

/* OTA state variables */
static struct k_work_delayable ota_check_work;
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
static int create_http_socket(const char *host, int port);
static void ota_enter_backoff_state(void);


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

/* Direkte Boot-Trailer Manipulation */
static int manual_boot_request_upgrade(void)
{
    const struct flash_area *fa;
    int ret;
    
    ret = flash_area_open(DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition)), &fa);
    if (ret) {
        LOG_ERR("Cannot open slot1: %d", ret);
        return ret;
    }
    
    /* Boot-Trailer am Ende von Slot1 schreiben */
    struct {
        uint8_t image_ok;
        uint8_t copy_done;
        uint8_t reserved[14];
        uint32_t magic;
    } __packed trailer = {
        .image_ok = 0x01,     // BOOT_FLAG_SET
        .copy_done = 0xFF,    // BOOT_FLAG_UNSET
        .magic = 0x96f3b83c   // BOOT_MAGIC
    };
    
    size_t trailer_offset = fa->fa_size - sizeof(trailer);
    
    ret = flash_area_write(fa, trailer_offset, &trailer, sizeof(trailer));
    if (ret) {
        LOG_ERR("Failed to write boot trailer: %d", ret);
        flash_area_close(fa);
        return ret;
    }
    
    LOG_INF("✓ Boot trailer written manually");
    flash_area_close(fa);
    return 0;
}

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
    uint8_t active_slot_id = boot_fetch_active_slot();

    rc = boot_read_bank_header(active_slot_id, &header, sizeof(header));
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

/* Create HTTP socket connection */
static int create_http_socket(const char *host, int port)
{
    struct zsock_addrinfo hints, *result;
    //struct sockaddr_in addr;
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
static int http_response_cb(struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
    int ret = 0;

    /* Check if this is the first time we get body data */
    if (!headers_complete && rsp->body_frag_len > 0) {
        if (rsp->http_status_code != 200) {
            LOG_ERR("HTTP request failed with status: %d %s", rsp->http_status_code, rsp->http_status);
            set_error(OTA_ERR_DOWNLOAD_FAILED);
            return -1;
        }
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
                int rc = ota_get_running_firmware_version(current_ver, sizeof(current_ver));
                if (strcmp(version.version, current_ver) != 0) {
                    LOG_INF("New version available: %s (current: %s)",
                           version.version, current_ver);
                    update_status(OTA_STATUS_UPDATE_AVAILABLE);
                    
                    /* Start download after a short delay */
                    k_work_schedule(&ota_check_work, K_SECONDS(5));
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

        if (current_status == OTA_STATUS_DOWNLOADING) {
            LOG_INF("Flash initialized for firmware download");
            total_downloaded = 0;
        }
    }
    
    /* Process body data for firmware download */
    if (headers_complete && current_status == OTA_STATUS_DOWNLOADING && 
        rsp->body_frag_len > 0) {
        
        ret = flash_img_buffered_write(&image_ctx, 
                rsp->body_frag_start,
                rsp->body_frag_len, 
                (final_data == HTTP_DATA_FINAL));

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
        }
    }
    
    return 0;
}

/* Check for firmware updates */
static int check_for_update(void)
{
    if (!wifi_is_connected()) {
        LOG_WRN("WiFi not connected (yet), skipping update check");
        return -ENOTCONN;
    }
    
    update_status(OTA_STATUS_CHECKING);
    
    /* try to create socket connection */
    http_sock = create_http_socket(OTA_SERVER_HOST, OTA_SERVER_PORT);
    if (http_sock < 0) {
        LOG_ERR("Server connection failed. Entering 1-hour backoff.");
        ota_enter_backoff_state();
        return http_sock;
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
    
    LOG_INF("Checking for updates at http://%s:%d%s", OTA_SERVER_HOST, OTA_SERVER_PORT, OTA_VERSION_URL);
    int ret = http_client_req(http_sock, &http_req, 5000, NULL); //blocks until done
    
    /* Close socket */
    zsock_close(http_sock);
    http_sock = -1;
    
    /* in case something went wrong */
    if (ret < 0) {
        LOG_ERR("Server connection failed. Entering 1-hour backoff.");
        ota_enter_backoff_state();
    }
    
    return ret;
}

/* Download and install firmware update */
static int perform_update(void)
{
    LOG_INF("Tryning to download firmware");

    if (!wifi_is_connected()) {
        LOG_WRN("WiFi not connected, cannot download update");
        ota_enter_backoff_state();
        return -ENOTCONN;
    }
    
    update_status(OTA_STATUS_DOWNLOADING);
    
    //int ret = flash_img_init(&image_ctx);
    int ret = flash_img_init_id(&image_ctx, DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition)));
    LOG_INF("area ID of slot1: %d", DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition)));

    if (ret != 0) {
        LOG_ERR("Failed to initialize flash context: %d", ret);
        set_error(OTA_ERR_FLASH_INIT);
        ota_enter_backoff_state();
        return ret;
    }

    uint8_t area_id = flash_img_get_upload_slot();
    LOG_INF("Flash image using area ID: %d", area_id);

    /* Create socket connection */
    http_sock = create_http_socket(OTA_SERVER_HOST, OTA_SERVER_PORT);
    if (http_sock < 0) {
        set_error(OTA_ERR_SERVER_CONNECT);
        LOG_ERR("Server connection failed. Entering 1-hour backoff.");
        ota_enter_backoff_state();
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
    total_downloaded = 0; 

    LOG_INF("Downloading firmware from http://%s:%d%s", OTA_SERVER_HOST, OTA_SERVER_PORT, OTA_FIRMWARE_URL);

    ret = http_client_req(http_sock, &http_req, OTA_DOWNLOAD_TIMEOUT_MS, NULL); // blocks until done

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
            ota_enter_backoff_state();
        }
    } else {
        LOG_INF("Firmware download successful.");
        update_status(OTA_STATUS_DOWNLOAD_COMPLETE);

        // The download is fully complete, and the HTTP client is closed.
        // The image_ctx is no longer in active use. NOW it is safe
        // to schedule the final "apply" step.
        k_work_schedule(&ota_check_work, K_MSEC(100)); // A short delay is fine.
        /* Reset retry counter on success */
        retry_count = 0;
    }
    
    return ret;
}

/* Apply downloaded update */
static int apply_update(void)
{
    update_status(OTA_STATUS_APPLYING);
    
    LOG_INF("=== Boot Status Debug ===");
    int swap_type = mcuboot_swap_type();
    LOG_INF("Current swap type: %d", swap_type);

    const struct flash_area *fa;
    uint32_t magic;
    int ret_dbg;
    
    
    /* Slot1 direkt aus Flash lesen */
    ret_dbg = flash_area_open(DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition)), &fa);
    if (ret_dbg != 0) {
        LOG_ERR("Cannot open slot1: %d", ret_dbg);
        return ret_dbg;
    }
    
    /* Erste 4 Bytes lesen (das ist der Magic-Wert) */
    ret_dbg = flash_area_read(fa, 0, &magic, sizeof(magic));
    if (ret_dbg == 0) {
        LOG_INF("Slot1 - First 4 bytes (magic): 0x%08x", magic);
    }

    debug_image_headers();

    /* Mark image for testing */
    int ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
    //int ret = manual_boot_request_upgrade();
    if (ret != 0) {
        LOG_ERR("Failed to request upgrade: %d", ret);
        set_error(OTA_ERR_APPLY_UPDATE);
        return ret;
    }
    
    LOG_INF("Update ready - rebooting in 3 seconds");
    k_sleep(K_SECONDS(3));
    sys_reboot(SYS_REBOOT_WARM); //SYS_REBOOT_COLD
    
    return 0; /* Will never reach here */
}

/* OTA check work handler */
static void ota_check_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    
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

static void ota_enter_backoff_state(void) {
    k_work_schedule(&ota_check_work, K_HOURS(1));
    set_error(OTA_ERR_SERVER_CONNECT);
    update_status(OTA_STATUS_IDLE);
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

int ota_mgmt_init(void)
{
    k_work_init_delayable(&ota_check_work, ota_check_work_handler);

    if (boot_is_img_confirmed()) {
        LOG_INF("Scheduling initial OTA check in 30 seconds.");
        k_work_schedule(&ota_check_work, K_SECONDS(30));
    }

    LOG_INF("OTA management subsystem initialized");
    return 0;
}

/* Auto-initialize at APPLICATION level */
SYS_INIT(ota_mgmt_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);