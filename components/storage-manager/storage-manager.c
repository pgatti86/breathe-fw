#include "storage-manager.h"

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"

static const char *SPIFFS_DEVICE_PARTITION = "device";

static const char *SPIFFS_SECURITY_PARTITION = "security";

static const char *TAG = "storage-manager";

static nvs_handle_t nvs_partition_handle;

static char* storage_manager_read_file_in_buffer(FILE* f) {

	/* quit if the file does not exist */
	if (f == NULL) {
		return NULL;
	}

	char  *buffer;
	long  numbytes;

	/* Get the number of bytes */
	fseek(f, 0L, SEEK_END);
	numbytes = ftell(f);

	/* reset the file position indicator to
	the beginning of the file */
	fseek(f, 0L, SEEK_SET);

	/* grab sufficient memory for the
	buffer to hold the text */
	buffer = (char*)calloc(numbytes, sizeof(char));

	/* memory error */
	if (buffer == NULL) {
		free(buffer);
		return NULL;
	}

	/* copy all the text into the buffer */
	fread(buffer, sizeof(char), numbytes, f);
	buffer[numbytes -1] = '\0';
	return buffer;
}

static char* storage_manager_read_file_from_spiffs(char *file_path) {

    FILE* f = fopen(file_path, "r");
	if (f == NULL) {
		ESP_LOGE(TAG, "Failed to open file for reading");
		return NULL;
	}

	char *buffer = storage_manager_read_file_in_buffer(f);
	fclose(f);

	return buffer;
}

bool storage_manager_init() {

    ESP_LOGI(TAG, "Initializing NVS");
    
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    err += nvs_open("storage", NVS_READWRITE, &nvs_partition_handle);

    ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t spiffs_device_conf = {
      .base_path = "/device",
      .partition_label = SPIFFS_DEVICE_PARTITION,
      .max_files = 5,
      .format_if_mount_failed = false
    };

    err += esp_vfs_spiffs_register(&spiffs_device_conf);

    esp_vfs_spiffs_conf_t spiffs_security_conf = {
      .base_path = "/security",
      .partition_label = SPIFFS_SECURITY_PARTITION,
      .max_files = 5,
      .format_if_mount_failed = false
    };

    err += esp_vfs_spiffs_register(&spiffs_security_conf);

    return err == ESP_OK;
}

char *storage_manager_read_file(char *file_path) {

    return storage_manager_read_file_from_spiffs(file_path);
}

bool storage_manager_get_prefs_bool_value(char *key, bool def_value) {
    
    uint8_t value = 0;
    esp_err_t ret = nvs_get_u8(nvs_partition_handle, key, &value);
    if (ret != ESP_OK) {
        return def_value;
    }
    
    return value;
}

bool storage_manager_set_prefs_bool_value(char *key, bool value) {

    uint8_t _value = value ? 1 : 0;
    esp_err_t err = nvs_set_u8(nvs_partition_handle, key, _value);
    err += nvs_commit(nvs_partition_handle);
    return err == ESP_OK;
}
