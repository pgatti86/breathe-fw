#include "ota-manager.h"

#include "esp_ota_ops.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define OTA_TIMER_PERIOD 60000000 // once an hour  3600000000

static const char *TAG = "ota-manager";

static EventGroupHandle_t ota_event_group;
const int OTA_CHECK_BIT = BIT0;

static void ota_manager_timer_callback(void* arg) {
    xEventGroupSetBits(ota_event_group, OTA_CHECK_BIT);
}

static esp_err_t ota_manager_http_event_handler(esp_http_client_event_t *evt) {
    
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        }
    return ESP_OK;
}

static void ota_task(void *args) {

    esp_http_client_config_t config = {
        .url = CONFIG_FW_UPDATE_URL,
        .event_handler = ota_manager_http_event_handler,
        .skip_cert_common_name_check = true
    };
    
    while (true) {

        xEventGroupWaitBits(ota_event_group, OTA_CHECK_BIT, true, true, portMAX_DELAY);
        
        ESP_LOGI(TAG, "Attempting to download update from %s", config.url);
        
        esp_err_t ret = esp_https_ota(&config);
    
        if (ret == ESP_OK) {
            esp_restart();
        } else {
            ESP_LOGI(TAG, "Firmware upgrade failed");
        }    
    }

    vTaskDelete(NULL);
}

int ota_manager_init() {

    ota_event_group = xEventGroupCreate();

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &ota_manager_timer_callback,
            .name = "ota-timer"
    };

    esp_timer_handle_t periodic_timer;
    esp_err_t ret = esp_timer_create(&periodic_timer_args, &periodic_timer);
    ret = esp_timer_start_periodic(periodic_timer, OTA_TIMER_PERIOD);

    if (ret == ESP_OK) {
        xTaskCreate(ota_task, "ota-task", 4096, NULL, 3, NULL);
    }

    return ret;
}

char* ota_manager_get_fw_version() {
    const esp_app_desc_t  *app_desc = esp_ota_get_app_description();
    return app_desc->version;
}

