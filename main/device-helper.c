#include "device-helper.h"

#include "storage-manager.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

static device_data_t *cached_device_data = NULL;

static const char *ENROLLMENT_KEY = "enrollment-key";

device_data_t* device_helper_get_device_config() {
    
    if (cached_device_data != NULL) {
        return cached_device_data;
    }

    char *file_data = storage_manager_read_file("/device/config.json");
    cJSON *json = cJSON_Parse(file_data);
    free(file_data);

    if (json == NULL) {
        cJSON_Delete(json);
        return NULL;
    }

    const cJSON *uid = cJSON_GetObjectItemCaseSensitive(json, "uid");
    cached_device_data = malloc(sizeof(device_data_t));
    strcpy(cached_device_data->uid, uid->valuestring);
    cJSON_Delete(json);

    return cached_device_data;
}

bool device_helper_is_enrollment_completed() {
    return storage_manager_get_prefs_bool_value(ENROLLMENT_KEY, false);
}

bool device_helper_set_enrollment_status(bool completed) {
    return storage_manager_set_prefs_bool_value(ENROLLMENT_KEY, completed);
}