#include "wifi-provisioning.h"
#include "wifi-provisioning-events.h"

#include <string.h>
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "esp_smartconfig.h"

#define MAX_CONNECTION_RETRY 2

static const char *TAG = "wifi-provisioning";

static const int CONNECTED_BIT = BIT0;

static const int ESPTOUCH_DONE_BIT = BIT1;

static EventGroupHandle_t s_wifi_event_group;

static smartconfig_start_config_t smart_config_cfg = SMARTCONFIG_START_CONFIG_DEFAULT();

static int connection_retry = 0;

ESP_EVENT_DEFINE_BASE(WIFI_MANAGER_PROVISIONING_EVENTS);

static void wifi_provisioning_send_event(int32_t event_id);

static void smartconfig_task(void *args) {
    EventBits_t uxBits;
    esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
    esp_smartconfig_start(&smart_config_cfg);
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY); 
        if (uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if (uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            wifi_provisioning_send_event(PROVISIONING_COMPLETED);
            vTaskDelete(NULL);
        }
    }
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
        if (connection_retry >= MAX_CONNECTION_RETRY) {
            esp_smartconfig_stop();
            connection_retry = 0;
            esp_smartconfig_start(&smart_config_cfg);
        } else {
            connection_retry += 1;
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        
        ESP_LOGI(TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
       
        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        
        /* Setting a password implies station will connect to all security modes including WEP/WPA.
         * However these modes are deprecated and not advisable to be used. In case your Access point
         * doesn't support WPA2, these mode can be enabled by commenting below line */
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        
        wifi_pmf_config_t pmf_cfg = {
            .capable = true,
            .required = false
        };
        wifi_config.sta.pmf_cfg = pmf_cfg;

        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_wifi_connect();

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

uint32_t wifi_provisioning_start() {

    s_wifi_event_group = xEventGroupCreate();
    esp_err_t result_code = esp_netif_init();

    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    result_code += esp_wifi_init(&cfg);

    result_code += esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);
    result_code += esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL);
    result_code += esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL);

    result_code += esp_wifi_set_mode(WIFI_MODE_STA);
    result_code += esp_wifi_start();

    return result_code;
}

static void wifi_provisioning_send_event(int32_t event_id) {
    esp_event_post(WIFI_MANAGER_PROVISIONING_EVENTS, event_id, NULL, 0, portMAX_DELAY);
}
