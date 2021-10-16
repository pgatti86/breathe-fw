#include "time-manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sntp.h"
#include "esp_log.h"

#define RETRY_CONT 20

static const char *TAG = "time-manager";

static bool is_sntp_initialized = false;

static void sync_time_task(void *args) {

    int retry = 0;

    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < RETRY_CONT) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, RETRY_CONT);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    
    char strftime_buf[ISO_DATE_LENGTH] = {'\0'};
    time_manager_format_time(strftime_buf, ISO_DATE_LENGTH);
    ESP_LOGI(TAG, "The current UTC date/time is: %s", strftime_buf);

    vTaskDelete(NULL);
}

void time_manager_sync_time() {

    if (time_manager_is_time_synched()) {
        return;
    }

    if (!is_sntp_initialized) {
        ESP_LOGI(TAG, "Initializing SNTP");
        is_sntp_initialized = true;
        sntp_setoperatingmode(SNTP_OPMODE_POLL);
        sntp_setservername(0, CONFIG_SNTP_SERVER);
        sntp_init();
    }

    xTaskCreate(sync_time_task, "sntp task", 2048, NULL, 5, NULL);
}

bool time_manager_is_time_synched() {
    
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    return timeinfo.tm_year > (2020 - 1900);
}

void time_manager_format_time(char *out, int length) {
    
    time_t now = 0;
    struct tm timeinfo = { 0 };
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(out, length, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
}