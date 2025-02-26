
#include "esp_log.h"
#include "esp_wifi.h"
#include <string.h>

#include "keys.c"

#define DEFAULT_SCAN_LIST_SIZE 10
#define DHCPS_OFFER_DNS 0x02

////////// DEBUG TAGS ///////////
static const char *STA_WIFI = "wifi STA";
static const char *AP_WIFI = "wifi AP";
static const char *NAPT = "wifi NAPT";

esp_netif_t *ap_netif;
esp_netif_t *sta_netif;


void print_network_interfaces() {
    esp_netif_t *netif = NULL;
    while ((netif = esp_netif_next(netif)) != NULL) {
        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(netif, &ip_info);
        
        ESP_LOGI("NETWORK", "Interface %p:", netif);
        ESP_LOGI("NETWORK", "  IP:      " IPSTR, IP2STR(&ip_info.ip));
        ESP_LOGI("NETWORK", "  Netmask: " IPSTR, IP2STR(&ip_info.netmask));
        ESP_LOGI("NETWORK", "  Gateway: " IPSTR, IP2STR(&ip_info.gw));
    }
}

void softap_set_dns_addr()
{
    esp_netif_dns_info_t dns;
    esp_netif_get_dns_info(sta_netif,ESP_NETIF_DNS_MAIN,&dns);
    // uint8_t dhcps_offer_option = DHCPS_OFFER_DNS;
    uint8_t dhcps_offer_option = 0;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(ap_netif));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(ap_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_option, sizeof(dhcps_offer_option)));
    ESP_ERROR_CHECK(esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(ap_netif));
}

void enable_nat(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
#ifdef NAPT_DEBUG
        ESP_LOGI(NAPT, "NAPT_DEBUG DEFINED !!!");
#endif
        // Add these debug prints in your IP_EVENT_STA_GOT_IP handler
        // esp_netif_ip_info_t sta_ip_info;
        // esp_netif_ip_info_t ap_ip_info;
        // esp_netif_get_ip_info(sta_netif, &sta_ip_info);
        // esp_netif_get_ip_info(ap_netif, &ap_ip_info);

        // ESP_LOGI(WIFI_TAG, "STA IP: " IPSTR ", GW: " IPSTR, IP2STR(&sta_ip_info.ip), IP2STR(&sta_ip_info.gw));
        // ESP_LOGI(WIFI_TAG, "AP  IP: " IPSTR ", GW: " IPSTR, IP2STR(&ap_ip_info.ip), IP2STR(&ap_ip_info.gw));

        ESP_LOGI(NAPT, "START ENABLE NAT FOR STA");
        // esp_netif_t *sta_netif = (esp_netif_t *)arg; // STA interface, not AP
        esp_netif_ip_info_t ip_info;
        
        esp_netif_get_ip_info(sta_netif, &ip_info);

        ESP_LOGI(NAPT, "STA IP Address: " IPSTR, IP2STR(&ip_info.ip));

        if (ip_info.ip.addr != 0) {
            
            //
            esp_netif_attach(sta_netif, esp_netif_get_io_driver(sta_netif));

            // Enable NAPT on the STA interface

            // Enable NAPT for the station interface
            esp_err_t err = esp_netif_napt_enable(sta_netif);
            // esp_err_t err = ESP_OK;
            if (err != ESP_OK) {
                if(err != 0x106)
                {
                    ESP_LOGE(NAPT, "esp_netif_napt_enable error -> ESP_ERR_NOT_SUPPORTED");
                    ESP_LOGW(NAPT, "you probably didn't enable napt in Menuconfig.");
                }
                ESP_LOGE(NAPT, "Failed to enable NAPT: %s", esp_err_to_name(err));
                return;
            }
            // print_network_interfaces();
            // napt_debug_print();

            ESP_LOGI(NAPT, "Added route for AP subnet via STA interface");

            // ip_napt_enable(ip_info.ip.addr, 1);
            
            
            // esp_err_t enab_nat_err = esp_netif_napt_enable(sta_netif);
            // if (enab_nat_err == ESP_OK) {
            //     ESP_LOGI(NAPT, "NAPT successfully enabled on STA interface");

            //     // Enable IP forwarding for the STA IP address
            // ip_napt_enable(ip4_addr_get_u32(&ip_info.ip), 1);
                // ip_napt_enable(ip_info.ip.addr, 1);
            // } else {
            //     ESP_LOGE(NAPT, "Failed to enable NAPT: %s", esp_err_to_name(enab_nat_err));
        } else {
            ESP_LOGW(NAPT, "No IP assigned to STA interface, cannot enable NAPT.");
        }
        // print_network_interfaces();
        ESP_LOGI(NAPT, "Setup DNS configuration");
        
        softap_set_dns_addr();

        esp_netif_ip_info_t sta_ip_info, ap_ip_info;
        esp_netif_get_ip_info(sta_netif, &sta_ip_info);
        esp_netif_get_ip_info(ap_netif, &ap_ip_info);
        ESP_LOGI(NAPT, "Routes - STA: " IPSTR "/24 via " IPSTR, IP2STR(&sta_ip_info.ip), IP2STR(&sta_ip_info.gw));
        ESP_LOGI(NAPT, "Routes - AP: " IPSTR "/24 via " IPSTR, IP2STR(&ap_ip_info.ip), IP2STR(&ap_ip_info.gw));
    }
}


void init_AP_STA()
{
    // init
    esp_netif_init();

    sta_netif = esp_netif_create_default_wifi_sta();
    ap_netif = esp_netif_create_default_wifi_ap();

    //set wifi mode
    esp_wifi_set_mode(WIFI_MODE_APSTA);

    //set config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_netif_set_hostname(sta_netif, "STA_HANDLE");

    wifi_config_t wifi_config_sta = {
        .sta = {
            .ssid = AP_NAME,
            .password = PSWD,
        },
    };
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta);



    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            // .authmode = WIFI_AUTH_WPA2_PSK
            .authmode = WIFI_AUTH_WPA2_WPA3_PSK
        },
    };
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap);

    // start wifi interfaces
    esp_err_t ap_result = esp_wifi_start();
    if (ap_result != ESP_OK)
    {
        ESP_LOGE(AP_WIFI, "WiFi AP start failed: %s", esp_err_to_name(ap_result));
    }else{
        ESP_LOGI(AP_WIFI, "WiFi AP started: %s", esp_err_to_name(ap_result));
    }

    // event handlers
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, enable_nat, NULL);

    // connect to source esp-sta to source AP
    esp_err_t sta_connect = esp_wifi_connect();
    if(sta_connect == ESP_OK)
    {
        ESP_LOGI(STA_WIFI, "Wifi sta connected successfully\n");
    }else{
        ESP_LOGI(STA_WIFI, "Wifi sta failed to connect\n");
    }

}