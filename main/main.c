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

static const char *TAG = "breathe-app";

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

    wifi_manager_init();
    mqtt_manager_init();
}
