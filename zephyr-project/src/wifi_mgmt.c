#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>
#include <string.h>
#include "app_config.h"
#include "wifi_mgmt.h"

LOG_MODULE_REGISTER(wifi_mgmt, LOG_LEVEL_INF);

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;  /* Zusätzlicher Callback für IP */
static struct k_work_delayable wifi_connect_work;
static bool wifi_connected = false;


/* WiFi event handler - mit IP-Abfrage */
static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                              uint64_t mgmt_event, struct net_if *iface)
{
    ARG_UNUSED(cb);
    ARG_UNUSED(iface);
    
    switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT:
        LOG_INF("WiFi connected successfully - checking for IP...");
        
        /* Kurz warten und dann IP abfragen */
        wifi_connected = true;
        k_work_schedule(&wifi_connect_work, K_SECONDS(2));
        break;
        
    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        LOG_WRN("WiFi disconnected - will retry");
        wifi_connected = false;
        k_work_schedule(&wifi_connect_work, K_SECONDS(5));
        break;
        
    default:
        break;
    }
}

/* Work handler auch für IP-Check nutzen */
static void wifi_connect_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    
    /* Wenn schon verbunden, IP-Adresse checken */
    if (wifi_connected) {
        char ip_str[16];
        if (wifi_get_ip_address(ip_str, sizeof(ip_str)) == 0) {
            LOG_INF("IP confirmed: %s", ip_str);
        }
        return;
    }
    
    /* Ansonsten WiFi-Connection versuchen */
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
    
    /* Nach WiFi-Connect erstmal als connected markieren */
    LOG_INF("Attempting WiFi connection to: %s", WIFI_SSID);
    
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
                      &params, sizeof(params));
    if (ret) {
        LOG_ERR("WiFi connection request failed: %d", ret);
        k_work_schedule(&wifi_connect_work, K_SECONDS(10));
    } else {
        /* Nach erfolgreichem Connect - IP-Check in 3 Sekunden */
        wifi_connected = true;
        k_work_schedule(&wifi_connect_work, K_SECONDS(3));
    }
}

/* IPv4 event handler - hier bekommen wir die IP-Adresse */
static void ipv4_event_handler(struct net_mgmt_event_callback *cb,
                              uint64_t mgmt_event, struct net_if *iface)
{
    ARG_UNUSED(cb);
    LOG_INF("IPv4 Event received: 0x%llx", mgmt_event);
    LOG_INF("rossi");

    if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        struct in_addr *ip_addr;  /* Korrekte Struktur */
        char ip_str[NET_IPV4_ADDR_LEN];
        
        ip_addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
        if (ip_addr) {
            net_addr_ntop(AF_INET, ip_addr, ip_str, sizeof(ip_str));
            LOG_INF("WiFi got IP address: %s", ip_str);
            wifi_connected = true;
        }
    }
    else if (mgmt_event == NET_EVENT_IPV4_ADDR_DEL) {
        LOG_INF("IPv4 address removed");
        wifi_connected = false;
    }
}

bool wifi_is_connected(void)
{
    return wifi_connected;
}

int wifi_get_ip_address(char *ip_str, size_t len)
{
    struct net_if *iface = net_if_get_default();
    struct in_addr *addr;
    
    if (!iface || !wifi_connected) {
        return -2;
    }
    
    LOG_INF("Checking for IP address...");
    
    /* Versuche PREFERRED Adresse */
    addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
    if (addr) {
        net_addr_ntop(AF_INET, addr, ip_str, len);
        LOG_INF("Found IP: %s", ip_str);
        return 0;
    }
    
    /* Versuche TENTATIVE Adresse als Fallback */
    addr = net_if_ipv4_get_global_addr(iface, NET_ADDR_TENTATIVE);
    if (addr) {
        net_addr_ntop(AF_INET, addr, ip_str, len);
        LOG_INF("Found tentative IP: %s", ip_str);
        return 0;
    }
    
    LOG_DBG("No IP address available yet");
    return -1;
}

static int wifi_mgmt_init(void)
{
    /* Setup WiFi event callback */
    net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler,
                                NET_EVENT_WIFI_CONNECT_RESULT |
                                NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);
    
    /* Setup IPv4 event callback */
    net_mgmt_init_event_callback(&ipv4_cb, ipv4_event_handler, 0xFFFFFFFFFFFFFFFF);

    net_mgmt_add_event_callback(&ipv4_cb);
    /* Initialize connection work */
    k_work_init_delayable(&wifi_connect_work, wifi_connect_work_handler);
    
    /* Start initial connection attempt */
    k_work_schedule(&wifi_connect_work, K_SECONDS(2));
    
    LOG_INF("WiFi management subsystem initialized");
    return 0;
}

/* Auto-initialize after networking */
SYS_INIT(wifi_mgmt_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);