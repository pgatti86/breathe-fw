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
#include "time-manager.h"
#include "wifi-provisioning-events.h"
#include "wifi-provisioning.h"
#include "storage-manager.h"
#include "device-helper.h"
#include "gpio-manager.h"
#include "driver/gpio.h"

static const char *TAG = "breathe-app";

static void pms_callback(pm_data_t *sensor_data) {

    data_sender_enqueue_pms_data(sensor_data);
}

static void load_mqtt_certificates(mqtt_certificates_t *out) {

    out->ca_cert = device_helper_get_ca_cert();
    out->device_cert = device_helper_get_device_cert();
    out->device_key = device_helper_get_device_key();
}

static void wifi_provisioning_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    
    if (id == PROVISIONING_COMPLETED) {
        device_helper_set_enrollment_status(COMPLETED);
        esp_restart();
        return;
    } 
}

static void wifi_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    
    if (id == WIFI_EVENT_CONNECTED) {
        time_manager_sync_time();
        mqtt_manager_connect();
        return;
    } 

    mqtt_manager_disconnect();
}

static void gpio_event_callback(uint8_t gpio_num, pad_event_t event) {
    
    if (gpio_num == GPIO_NUM_0 && event == GPIO_LONG_CLICK) {
        storage_manager_reset();
        esp_restart();
    }
}

static void main_task(void *args) {

    esp_event_handler_register(WIFI_MANAGER_EVENTS, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    
     pad_conf_t reset_pad_conf = {
        .gpio_number = GPIO_NUM_0,
        .direction = GPIO_INPUT,
        .pull_mode = GPIO_PULL_UP,
        .interrput_mode = GPIO_INTERRUPT_FALLING,
        .callback = &gpio_event_callback
    };

    gpio_manager_init();
    gpio_manager_configure_pad(&reset_pad_conf);

    wifi_manager_init();
    mqtt_manager_init(&load_mqtt_certificates);
    data_sender_init();

    pms_conf.callback = &pms_callback,
    idf_pmsx5003_init(&pms_conf);

    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void app_main(void) {

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

    storage_manager_init();

    if (device_helper_is_enrollment_completed()) {
        xTaskCreate(main_task, "main task", 4096, NULL, 5, NULL);
    } else {
        esp_event_handler_register(WIFI_MANAGER_PROVISIONING_EVENTS, ESP_EVENT_ANY_ID, wifi_provisioning_event_handler, NULL);
        wifi_provisioning_start();
    }
}
