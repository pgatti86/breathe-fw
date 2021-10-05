#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi-events.h"
#include "wifi-manager.h"
#include "mqtt-manager.h"
#include "idf-pmsx003.h"
#include "data-sender.h"
#include "pmsx-config.h"
#include "device-helper.h"

static const char *TAG = "breathe-app";

static void pms_callback(pm_data_t *sensor_data) {
    ESP_LOGI(TAG, "pm10: %d ug/m3", sensor_data->pm10);
    ESP_LOGI(TAG, "pm2.5: %d ug/m3", sensor_data->pm2_5);
    ESP_LOGI(TAG, "pm1.0: %d ug/m3", sensor_data->pm1_0);
    ESP_LOGI(TAG, "particles > 0.3um / 0.1L: %d", sensor_data->particles_03um);
    ESP_LOGI(TAG, "particles > 0.5um / 0.1L: %d", sensor_data->particles_05um);
    ESP_LOGI(TAG, "particles > 1.0um / 0.1L: %d", sensor_data->particles_10um);
    ESP_LOGI(TAG, "particles > 2.5um / 0.1L: %d", sensor_data->particles_25um);
    ESP_LOGI(TAG, "particles > 5.0um / 0.1L: %d", sensor_data->particles_50um);
    ESP_LOGI(TAG, "particles > 10.0um / 0.1L: %d", sensor_data->particles_100um);

    char *device_id = device_helper_get_device_id();
    data_sender_send_pms_data(device_id, sensor_data);
}

static void wifi_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    if (id == WIFI_EVENT_CONNECTED) {
        mqtt_manager_connect();
        return;
    } 

    mqtt_manager_disconnect();
}

void app_main(void) {

    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");        
    fflush(stdout);

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    
    esp_event_loop_create_default();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      nvs_flash_erase();
      nvs_flash_init();
    }
    
    // Register for Wifi events
    esp_event_handler_register(WIFI_MANAGER_EVENTS, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);

    pms_conf.callback = &pms_callback,
    idf_pmsx5003_init(&pms_conf);

    wifi_manager_init();
    mqtt_manager_init();
}
