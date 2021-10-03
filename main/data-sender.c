#include "data-sender.h"

#include "mqtt-manager.h"
#include "cJSON.h"

static const char *PMS_TELEMETRY_DATA_TOPIC = "PG00001/pollution";

static cJSON* data_sender_prepare_pms_message(pm_data_t *data) {
    cJSON *pms_data = cJSON_CreateObject();

    cJSON *pm1_0 = cJSON_CreateNumber(data->pm1_0);
    cJSON_AddItemToObject(pms_data, "pm1.0", pm1_0);
    
    cJSON *pm2_5 = cJSON_CreateNumber(data->pm2_5);
    cJSON_AddItemToObject(pms_data, "pm2.5", pm2_5);
    
    cJSON *pm10 = cJSON_CreateNumber(data->pm10);
    cJSON_AddItemToObject(pms_data, "pm10", pm10);

    return pms_data;
}

bool data_sender_send_pms_data(pm_data_t *data) {
    
    cJSON *json_data = data_sender_prepare_pms_message(data);
    const char *out = cJSON_PrintUnformatted(json_data);
    cJSON_Delete(json_data);
    
    return mqtt_manager_publish(PMS_TELEMETRY_DATA_TOPIC, out);
}

