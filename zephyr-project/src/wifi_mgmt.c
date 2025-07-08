#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dhcpv4.h>  // DHCP Header hinzufügen
#include <zephyr/net/socket.h>  // Für Socket-Funktionen
#include <zephyr/logging/log.h>
#include <zephyr/init.h>
#include <string.h>
#include "app_config.h"

LOG_MODULE_REGISTER(wifi_mgmt, LOG_LEVEL_INF);

static struct net_mgmt_event_callback wifi_cb;
static struct k_work_delayable wifi_connect_work;
static bool wifi_connected = false;

/* Forward declarations */
static int wifi_get_ip_address(char *ip_str, size_t len);
static void wifi_connect_work_handler(struct k_work *work);

/* Interface setup nach WiFi-Connect */
static void setup_network_interface(struct net_if *iface)
{
    if (!iface) {
        LOG_ERR("No interface provided");
        return;
    }
    
    LOG_INF("=== Setting up network interface ===");
    
    /* Interface Status prüfen */
    LOG_INF("Interface up before: %s", net_if_is_up(iface) ? "YES" : "NO");
    LOG_INF("Interface admin up: %s", net_if_is_admin_up(iface) ? "YES" : "NO");
    
    /* Interface explizit aktivieren */
    if (!net_if_is_up(iface)) {
        LOG_INF("Bringing interface UP");
        net_if_up(iface);
    }
    
    /* DHCP explizit starten */
    LOG_INF("Starting DHCP client");
    net_dhcpv4_start(iface);
    
    LOG_INF("Interface up after: %s", net_if_is_up(iface) ? "YES" : "NO");
}

static int test_network_connectivity(void)
{
    struct sockaddr_in addr;
    int sock;
    int ret;
    
    /* Versuche Verbindung zu Google DNS (8.8.8.8) */
    sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        LOG_ERR("Failed to create socket: %d", errno);
        return -1;
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53);  // DNS Port
    zsock_inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);
    
    LOG_INF("Testing connectivity to 8.8.8.8:53...");
    
    ret = zsock_connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    zsock_close(sock);
    
    if (ret == 0) {
        LOG_INF("✓ Internet connectivity confirmed!");
        return 0;
    } else {
        LOG_WRN("✗ Internet connectivity failed: %d", ret);
        return -1;
    }
}

/* IP-Adresse Debug-Funktion */
static int wifi_get_ip_address(char *ip_str, size_t len)
{
    struct net_if *iface = net_if_get_default();
    
    if (!iface || !wifi_connected) {
        return -2;
    }
    
    LOG_INF("=== IP Address Debug ===");
    LOG_INF("Interface up: %s", net_if_is_up(iface) ? "YES" : "NO");
    
    /* Alle IPv4-Adressen durchsuchen */
    for (int state = 0; state < 5; state++) {
        struct in_addr *addr = net_if_ipv4_get_global_addr(iface, state);
        if (addr) {
            char temp_ip[16];
            net_addr_ntop(AF_INET, addr, temp_ip, sizeof(temp_ip));
            LOG_INF("Found IP (state %d): %s", state, temp_ip);
            
            /* Jede gefundene IP verwenden */
            strncpy(ip_str, temp_ip, len);
            ip_str[len-1] = '\0';
            return 0;
        }
    }
    
    LOG_WRN("No IP addresses found");
    return -1;
}

/* WiFi connection work handler */
static void wifi_connect_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    
    /* Wenn schon verbunden, IP-Adresse checken */
    if (wifi_connected) {
        LOG_INF("WiFi connected - checking for IP address...");
        char ip_str[16];
        int result = wifi_get_ip_address(ip_str, sizeof(ip_str));
        
        if (result == 0) {
            LOG_INF("*** IP address obtained: %s ***", ip_str);

            /* Netzwerk-Konnektivität testen */
            if (test_network_connectivity() == 0) {
                LOG_INF("*** Full network connectivity confirmed! ***");
            }

            return; // Erfolgreich - fertig
        } else {
            LOG_INF("Still waiting for IP address... (retry in 5s)");
            k_work_schedule(&wifi_connect_work, K_SECONDS(5));
            return;
        }
    }
    
    /* WiFi-Connection versuchen */
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("WiFi interface not available");
        k_work_schedule(&wifi_connect_work, K_SECONDS(5));
        return;
    }
    
    struct wifi_connect_req_params params = {
        .ssid = WIFI_SSID,
        .ssid_length = strlen(WIFI_SSID),
        .psk = WIFI_PSK,
        .psk_length = strlen(WIFI_PSK),
        .channel = WIFI_CHANNEL_ANY,
        .security = WIFI_SECURITY_TYPE_PSK,
    };
    
    LOG_INF("Attempting WiFi connection to: %s", WIFI_SSID);
    
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
                      &params, sizeof(params));
    
    if (ret) {
        LOG_ERR("WiFi connection request failed: %d", ret);
        k_work_schedule(&wifi_connect_work, K_SECONDS(10));
    }
}

/* WiFi event handler */
static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                              uint64_t mgmt_event, struct net_if *iface)
{
    ARG_UNUSED(cb);
    
    switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT:
        LOG_INF("*** WiFi connected successfully ***");
        wifi_connected = true;
        
        /* Network Interface setup */
        setup_network_interface(iface);
        
        /* Etwas länger warten für DHCP */
        k_work_schedule(&wifi_connect_work, K_SECONDS(10));
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

/* Public functions */
bool wifi_is_connected(void)
{
    return wifi_connected;
}

int wifi_get_ip_address_public(char *ip_str, size_t len)
{
    return wifi_get_ip_address(ip_str, len);
}

static int wifi_mgmt_init(void)
{
    /* Setup WiFi event callback */
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