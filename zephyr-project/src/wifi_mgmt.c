#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>
#include <string.h>
#include "app_config.h"

LOG_MODULE_REGISTER(wifi_mgmt, LOG_LEVEL_INF);

static struct net_mgmt_event_callback wifi_cb;
static struct k_work_delayable wifi_connect_work;
static bool wifi_connected = false;

/* WiFi event handler - Korrigierte Signatur */
static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                              uint64_t mgmt_event, struct net_if *iface)
{
    ARG_UNUSED(cb);
    ARG_UNUSED(iface);
    
    switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT:
        LOG_INF("WiFi connected successfully");
        wifi_connected = true;
        break;
        
    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        LOG_WRN("WiFi disconnected - will retry");
        wifi_connected = false;
        /* Schedule reconnection */
        k_work_schedule(&wifi_connect_work, K_SECONDS(5));
        break;
        
    default:
        break;
    }
}

/* WiFi connection work handler */
static void wifi_connect_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    
    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params params = {
        .ssid = WIFI_SSID,
        .ssid_length = strlen(WIFI_SSID),
        .psk = WIFI_PSK,
        .psk_length = strlen(WIFI_PSK),
        .channel = WIFI_CHANNEL_ANY,
        .security = WIFI_SECURITY_TYPE_PSK,
    };
    
    if (!iface) {
        LOG_ERR("WiFi interface not available");
        return;
    }
    
    LOG_INF("Attempting WiFi connection to: %s", WIFI_SSID);
    
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
                      &params, sizeof(params));
    if (ret) {
        LOG_ERR("WiFi connection request failed: %d", ret);
        /* Retry after delay */
        k_work_schedule(&wifi_connect_work, K_SECONDS(10));
    }
}

bool wifi_is_connected(void)
{
    return wifi_connected;
}

static int wifi_mgmt_init(void)
{
    /* Setup event callback */
    net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler,
                                NET_EVENT_WIFI_CONNECT_RESULT |
                                NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);
    
    /* Initialize connection work */
    k_work_init_delayable(&wifi_connect_work, wifi_connect_work_handler);
    
    /* Start initial connection attempt */
    k_work_schedule(&wifi_connect_work, K_SECONDS(2));
    
    LOG_INF("WiFi management subsystem initialized");
    return 0;
}

/* Auto-initialize after networking */
SYS_INIT(wifi_mgmt_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);