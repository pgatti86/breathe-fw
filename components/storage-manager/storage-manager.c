#include "storage-manager.h"

#include "esp_system.h"
#include "nvs_flash.h"

#define ENROLLMENT_KEY "enrollment-key"

static nvs_handle_t nvs_partition_handle;

bool storage_manager_init() {

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    err += nvs_open("storage", NVS_READWRITE, &nvs_partition_handle);
    return err == ESP_OK;
}

bool storage_manager_set_enrollment_status(bool completed) {
    
    uint8_t value = completed ? 1 : 0;
    esp_err_t err = nvs_set_u8(nvs_partition_handle, ENROLLMENT_KEY, value);
    err += nvs_commit(nvs_partition_handle);
    return err == ESP_OK;
}

bool storage_manager_has_enrollment_done() {

    uint8_t has_enrollment_done = 0;
    nvs_get_u8(nvs_partition_handle, ENROLLMENT_KEY, &has_enrollment_done);
    
    return has_enrollment_done;
}