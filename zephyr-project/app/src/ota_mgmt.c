#include "wifi_mgmt.h"
#include "app_config.h"
#include "ota_mgmt.h"
#include "blinky.h"
#include "utils.h"

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

struct version_info {
    const char *version;
    int size;
};

static struct json_obj_descr version_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct version_info, version, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct version_info, size, JSON_TOK_NUMBER),
};

// Forward declarations
static int ota_mgmt_init(void);
static void ota_check_work_handler(struct k_work *work);
static int check_for_update(void);
static int download_update(void);
static int apply_update(void);

static void update_status(ota_status_t new_status);
static void set_error(ota_error_t error);
static void ota_enter_backoff_state(void);

static int handle_http_headers(struct http_response *rsp);
static int handle_version_response(struct http_response *rsp);
static int handle_firmware_download(struct http_response *rsp, enum http_final_call final_data);
static int process_version_info(const char *json_data, size_t len);
static int write_firmware_chunk(const char *data, size_t len, bool is_final);
static int create_http_socket(const char *host, int port);

// Public functions
int ota_check_for_update(void)
{
    if (current_status == OTA_STATUS_SLEEPING) {
        LOG_INF("Waking ota manager up..");
        update_status(OTA_STATUS_IDLE);
    }
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

// private static functions
static int http_response_cb(struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
    int ret = 0;
    if (!headers_complete && rsp->body_frag_len > 0) {
        ret = handle_http_headers(rsp);
        if (ret != 0) {
            return ret;
        }
    }

    if (headers_complete && rsp->body_frag_len > 0) {
        switch (current_status) {
            case OTA_STATUS_CHECKING:
                return handle_version_response(rsp);
                
            case OTA_STATUS_DOWNLOADING:
                return handle_firmware_download(rsp, final_data);
                
            default:
                LOG_WRN("Unexpected status in HTTP callback: %d", current_status);
                return 0;
        }
    }

    return 0;
}

static int handle_http_headers(struct http_response *rsp)
{
    if (rsp->http_status_code != 200) {
        LOG_ERR("HTTP request failed with status: %d %s", rsp->http_status_code, rsp->http_status);
        set_error(OTA_ERR_DOWNLOAD_FAILED);
        return -1;
    }
    
    headers_complete = true;
    content_length = rsp->content_length;
    LOG_INF("HTTP headers complete, content length: %lld", content_length);
    return 0;
}

static int handle_version_response(struct http_response *rsp)
{
    return process_version_info(rsp->body_frag_start, rsp->body_frag_len);
}

static int handle_firmware_download(struct http_response *rsp, enum http_final_call final_data)
{
    if (total_downloaded == 0) {
        LOG_INF("Flash initialized for firmware download");
    }
    
    return write_firmware_chunk(rsp->body_frag_start, rsp->body_frag_len, (final_data == HTTP_DATA_FINAL));
}

static int process_version_info(const char *json_data, size_t len)
{
    struct version_info version = {0};
    int ret;
    ret = json_obj_parse((char *)json_data, len, version_descr, ARRAY_SIZE(version_descr), &version);
    if (ret <= 0 || !version.version) {
        LOG_ERR("Failed to parse version info");
        set_error(OTA_ERR_SERVER_CONNECT);
        return -1;
    }
    
    LOG_INF("Server version: %s", version.version);

    char current_ver[16];
    ota_get_running_firmware_version(current_ver, sizeof(current_ver));
    
    if (strcmp(version.version, current_ver) != 0) {
        LOG_INF("New version available: %s (current: %s)", version.version, current_ver);
        update_status(OTA_STATUS_UPDATE_AVAILABLE);
        k_work_schedule(&ota_check_work, K_SECONDS(5));
    } else {
        LOG_INF("Already running latest version. Checking again later.");
        ota_enter_backoff_state();
    }
    
    return 0;
}

static int write_firmware_chunk(const char *data, size_t len, bool is_final)
{
    int ret;
    
    ret = flash_img_buffered_write(&image_ctx, data, len, is_final);
    if (ret < 0) {
        LOG_ERR("Flash write error: %d", ret);
        set_error(OTA_ERR_FLASH_WRITE);
        return ret;
    }
    
    total_downloaded += len;

    if (is_final) {
        LOG_INF("Firmware download complete: %zu bytes", total_downloaded);
    }
    
    return 0;
}

static int create_http_socket(const char *host, int port)
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

static void ota_enter_backoff_state(void) {
    set_error(OTA_ERR_NONE);
    update_status(OTA_STATUS_SLEEPING);
    LOG_WRN("Entering sleeping state.");
    k_work_schedule(&ota_check_work, K_SECONDS(OTA_CHECK_INTERVAL_SEC));
}

static void update_status(ota_status_t new_status)
{
    if (current_status != new_status) {
        current_status = new_status;
        
        if (status_callback != NULL) {
            status_callback(new_status);
        }
    }
}

static void set_error(ota_error_t error)
{
    last_error = error;
    update_status(OTA_STATUS_ERROR);
}

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
        LOG_ERR("Server connection failed");
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
        LOG_ERR("HTTP Request returned an error.");
    }
    
    return ret;
}

static int download_update(void)
{
    LOG_INF("Trying to download firmware, preparing flash area");

    if (!wifi_is_connected()) {
        LOG_WRN("WiFi not connected, cannot download update.");
        return -ENOTCONN;
    }

    const struct flash_area *fa;
    int ret;

    LOG_INF("Explicitly erasing slot1 before download...");
    ret = flash_area_open(DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition)), &fa);
    if (ret != 0) {
        LOG_ERR("Failed to open slot1 for erase: %d", ret);
        set_error(OTA_ERR_FLASH_INIT);
        return ret;
    }
    ret = flash_area_erase(fa, 0, fa->fa_size); // TODO: maybe remove -> deleting the whole flash area
    flash_area_close(fa);

    if (ret != 0) {
        LOG_ERR("Failed to erase slot1: %d", ret);
        set_error(OTA_ERR_FLASH_INIT);
        return ret;
    }
    LOG_INF("Slot1 cleared successfully.");
    ret = flash_img_init_id(&image_ctx, DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition)));
    LOG_INF("area ID of slot1: %d", DT_FIXED_PARTITION_ID(DT_NODELABEL(slot1_partition)));

    if (ret != 0) {
        LOG_ERR("Failed to initialize flash context: %d", ret);
        set_error(OTA_ERR_FLASH_INIT);
        return ret;
    }

    uint8_t area_id = flash_img_get_upload_slot();
    LOG_INF("Flash image using area ID: %d", area_id);
    
    update_status(OTA_STATUS_DOWNLOADING);
    http_sock = create_http_socket(OTA_SERVER_HOST, OTA_SERVER_PORT);
    if (http_sock < 0) {
        set_error(OTA_ERR_SERVER_CONNECT);
        LOG_ERR("Server connection failed.");
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
    zsock_close(http_sock);
    http_sock = -1;
    
    if (ret < 0) {
        LOG_ERR("Failed to download firmware: %d", ret);
        set_error(OTA_ERR_DOWNLOAD_FAILED);

        if (++retry_count < OTA_MAX_DOWNLOAD_RETRIES) {
            LOG_INF("Retrying download (%d/%d) in 5 seconds...", retry_count + 1, OTA_MAX_DOWNLOAD_RETRIES);
            k_work_schedule(&ota_check_work, K_SECONDS(5));
        } else {
            LOG_ERR("Max retry attempts reached, giving up");
            retry_count = 0;
            update_status(OTA_STATUS_IDLE);
            return -1;
        }
    } else {
        LOG_INF("Firmware download successful.");
        update_status(OTA_STATUS_DOWNLOAD_COMPLETE);
        k_work_schedule(&ota_check_work, K_MSEC(100));
        retry_count = 0;
    }
    
    return ret;
}

static int apply_update(void)
{
    update_status(OTA_STATUS_APPLYING);
    int swap_type = mcuboot_swap_type();
    LOG_INF("Current swap type: %d", swap_type);

    debug_image_headers(); // this should print an overview of the images in slot0 and slot1

    int ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
    if (ret != 0) {
        LOG_ERR("Failed to request upgrade: %d", ret);
        set_error(OTA_ERR_APPLY_UPDATE);
        return ret;
    }
    
    LOG_INF("Update ready - rebooting in 3 seconds");
    k_sleep(K_SECONDS(3));
    sys_reboot(SYS_REBOOT_WARM); //SYS_REBOOT_COLD -> no change because the signal bytes are stored anyways
    
    return 0;
}

static void ota_check_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    int ret = 0;

    switch (current_status) {
        case OTA_STATUS_IDLE:
            ret = check_for_update();
            break;
            
        case OTA_STATUS_UPDATE_AVAILABLE:
            ret = download_update();
            break;
            
        case OTA_STATUS_DOWNLOAD_COMPLETE:
            ret = apply_update();
            break;
        case OTA_STATUS_SLEEPING:
            update_status(OTA_STATUS_IDLE);
            ret = check_for_update();
            break;
        default:
            break;
    }

    if(ret < 0) {
        ota_enter_backoff_state();
    }
}

static int ota_mgmt_init(void)
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