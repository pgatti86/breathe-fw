#include "data-sender.h"

#include "mqtt-manager.h"
#include "device-helper.h"
#include "time-manager.h"
#include "cJSON.h"

static const char *POLLUTION_TELEMETRY_TEMPLATE_TOPIC = "%s/telemetry/pollution";

static cJSON* data_sender_prepare_pms_message(char *device_id, pm_data_t *data) {

    cJSON *pms_data = cJSON_CreateObject();

    cJSON *deviceId = cJSON_CreateString(device_id);
    cJSON_AddItemToObject(pms_data, "deviceId", deviceId);

    cJSON *pm1_0 = cJSON_CreateNumber(data->pm1_0);
    cJSON_AddItemToObject(pms_data, "pm1.0", pm1_0);
    
    cJSON *pm2_5 = cJSON_CreateNumber(data->pm2_5);
    cJSON_AddItemToObject(pms_data, "pm2.5", pm2_5);
    
    cJSON *pm10 = cJSON_CreateNumber(data->pm10);
    cJSON_AddItemToObject(pms_data, "pm10", pm10);

    cJSON *particles0_3 = cJSON_CreateNumber(data->particles_03um);
    cJSON_AddItemToObject(pms_data, "particlesCount0.3", particles0_3);

    cJSON *particles0_5 = cJSON_CreateNumber(data->particles_05um);
    cJSON_AddItemToObject(pms_data, "particlesCount0.5", particles0_5);

    cJSON *particles1_0 = cJSON_CreateNumber(data->particles_10um);
    cJSON_AddItemToObject(pms_data, "particlesCount1.0", particles1_0);

    cJSON *particles2_5 = cJSON_CreateNumber(data->particles_25um);
    cJSON_AddItemToObject(pms_data, "particlesCount2.5", particles2_5);

    cJSON *particles5_0 = cJSON_CreateNumber(data->particles_50um);
    cJSON_AddItemToObject(pms_data, "particlesCount5.0", particles5_0);

    cJSON *particles10_0 = cJSON_CreateNumber(data->particles_100um);
    cJSON_AddItemToObject(pms_data, "particlesCount10.0", particles10_0);

    if (time_manager_is_time_synched()) {
        
        char timestamp[ISO_DATE_LENGTH] = {'\0'};
        time_manager_format_time(timestamp, ISO_DATE_LENGTH);
        cJSON *t = cJSON_CreateString(timestamp);
        cJSON_AddItemToObject(pms_data, "timestamp", t);
    }

    return pms_data;
}

bool data_sender_send_pms_data(pm_data_t *data) {
    
    char *device_id = device_helper_get_device_id();

    cJSON *json_data = data_sender_prepare_pms_message(device_id, data);
    const char *out = cJSON_PrintUnformatted(json_data);
    cJSON_Delete(json_data);
    
    char topic[40] = {'\0'};
    sprintf(topic, POLLUTION_TELEMETRY_TEMPLATE_TOPIC, device_id);

    return mqtt_manager_publish(topic, out);
}

