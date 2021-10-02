#include "wifi-manager.h"
#include "wifi-events.h"

#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "freertos/task.h"

#define WIFI_RETRY_CONNECTION_BIT BIT0

#define CONNECTION_RETRY_DELAY 1000

#define CONNECTION_MAX_RETRY_DELAY 60000

static const char *TAG = "wifi-manager";

static EventGroupHandle_t s_wifi_event_group;

static int connection_retry_count = 0;

static bool is_connected = false;

static void wifi_manager_send_event(int32_t event_id);

ESP_EVENT_DEFINE_BASE(WIFI_MANAGER_EVENTS);

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "connect to the AP");
        xEventGroupSetBits(s_wifi_event_group, WIFI_RETRY_CONNECTION_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "retry to connect to the AP");
        is_connected = false;
        wifi_manager_send_event(WIFI_EVENT_DISCONNECTED);
        xEventGroupSetBits(s_wifi_event_group, WIFI_RETRY_CONNECTION_BIT);
        connection_retry_count++;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        is_connected = true;
        wifi_manager_send_event(WIFI_EVENT_CONNECTED);
        connection_retry_count = 0;
    }
}

static void wifi_manager_reconnection_task(void *ctx) {

    while (1) {
        xEventGroupWaitBits(s_wifi_event_group, WIFI_RETRY_CONNECTION_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        
        int incremental_delay = connection_retry_count * CONNECTION_RETRY_DELAY;
        int delay = incremental_delay > CONNECTION_MAX_RETRY_DELAY ? CONNECTION_MAX_RETRY_DELAY : incremental_delay;
        vTaskDelay(delay / portTICK_RATE_MS);
        
        if (!is_connected) {
            ESP_LOGI(TAG, "connection attemp no. %d", connection_retry_count);
            esp_wifi_connect();
        }
    }
    
    vTaskDelete(NULL);
}

esp_err_t wifi_manager_init(void) {

    s_wifi_event_group = xEventGroupCreate();
    esp_err_t result_code = esp_netif_init();

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    result_code += esp_wifi_init(&cfg);

    result_code += esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    result_code += esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_SSID,
            .password = CONFIG_PASSWORD,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. In case your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	        .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    result_code += esp_wifi_set_mode(WIFI_MODE_STA);
    result_code += esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    
    if (result_code == ESP_OK) {
        xTaskCreate(wifi_manager_reconnection_task, "wifi reconnect task", 2048, NULL, 5, NULL);
        result_code += esp_wifi_start();
    }
    
    return result_code;
}

bool wifi_manager_is_connected() {
    return is_connected;
}

static void wifi_manager_send_event(int32_t event_id) {
    esp_event_post(WIFI_MANAGER_EVENTS, event_id, NULL, 0, portMAX_DELAY);
}

void wifi_manager_deinit(void) {
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler);
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    vEventGroupDelete(s_wifi_event_group);
    esp_wifi_disconnect();
    esp_wifi_stop();
}

