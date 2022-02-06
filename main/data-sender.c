#include "data-sender.h"

#include "mqtt-manager.h"
#include "device-helper.h"
#include "time-manager.h"
#include "app-models.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include  "freertos/queue.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "data-sender";

static const char *PROVISIONING_TEMPLATE_TOPIC = "%s/provisioning";

static const char *POLLUTION_TELEMETRY_TEMPLATE_TOPIC = "%s/telemetry/pollution";

static const char *TEMPERATURE_TELEMETRY_TEMPLATE_TOPIC = "%s/telemetry/temperature";

static const char *HUMIDITY_TELEMETRY_TEMPLATE_TOPIC = "%s/telemetry/humidity";

static QueueHandle_t mqtt_pms_data_output_queue;

static void data_sender_init_json_message(cJSON *message, char *device_id) {

    cJSON *deviceId = cJSON_CreateString(device_id);
    cJSON_AddItemToObject(message, "deviceId", deviceId);

    if (time_manager_is_time_synched()) {
    
        char timestamp[ISO_DATE_LENGTH] = {'\0'};
        time_manager_format_time(timestamp, ISO_DATE_LENGTH);
        cJSON *t = cJSON_CreateString(timestamp);
        cJSON_AddItemToObject(message, "timestamp", t);
    }
}

static bool data_sender_publish_message(const char *topic_template, char *device_uid, char *payload) {

    char topic[80] = {'\0'};
    sprintf(topic, topic_template, device_uid);
    return mqtt_manager_publish(topic, payload);
}

static cJSON* data_sender_prepare_provisioning_message(char *device_id) {

    cJSON *provisioning_data = cJSON_CreateObject();

    data_sender_init_json_message(provisioning_data, device_id);

    cJSON *properties = cJSON_CreateArray();
    cJSON_AddItemToObject(provisioning_data, "properties", properties);

    cJSON *capabilities = cJSON_CreateArray();
    cJSON_AddItemToObject(provisioning_data, "capabilities", capabilities);

    return provisioning_data;
}

static void data_sender_log_pms_data(pm_data_t *sensor_data) {

    ESP_LOGI(TAG, "pm10: %d ug/m3", sensor_data->pm10);
    ESP_LOGI(TAG, "pm2.5: %d ug/m3", sensor_data->pm2_5);
    ESP_LOGI(TAG, "pm1.0: %d ug/m3", sensor_data->pm1_0);
    ESP_LOGI(TAG, "particles > 0.3um / 0.1L: %d", sensor_data->particles_03um);
    ESP_LOGI(TAG, "particles > 0.5um / 0.1L: %d", sensor_data->particles_05um);
    ESP_LOGI(TAG, "particles > 1.0um / 0.1L: %d", sensor_data->particles_10um);
    ESP_LOGI(TAG, "particles > 2.5um / 0.1L: %d", sensor_data->particles_25um);
    ESP_LOGI(TAG, "particles > 5.0um / 0.1L: %d", sensor_data->particles_50um);
    ESP_LOGI(TAG, "particles > 10.0um / 0.1L: %d", sensor_data->particles_100um);
}

static cJSON* data_sender_prepare_pms_message(char *device_id, pm_data_t *data) {

    cJSON *pms_data = cJSON_CreateObject();

    data_sender_init_json_message(pms_data, device_id);

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

    return pms_data;
}

static cJSON* data_sender_prepare_temperature_message(char *device_id, float *temperature) {

    cJSON *temperature_data = cJSON_CreateObject();

    data_sender_init_json_message(temperature_data, device_id);

    cJSON *t = cJSON_CreateNumber(*temperature);
    cJSON_AddItemToObject(temperature_data, "temperature", t);

    return temperature_data;
}

static cJSON* data_sender_prepare_humidity_message(char *device_id, float *humidity) {

    cJSON *humidity_data = cJSON_CreateObject();

    data_sender_init_json_message(humidity_data, device_id);
    
    cJSON *h = cJSON_CreateNumber(*humidity);
    cJSON_AddItemToObject(humidity_data, "humidity", h);
    
    return humidity_data;
}

static void pms_data_sender_task(void *args) {

    while (true) {
        
        pm_data_t pms_data;
        xQueueReceive(mqtt_pms_data_output_queue, &pms_data, portMAX_DELAY);

        data_sender_log_pms_data(&pms_data);

        device_data_t *device_data = device_helper_get_device_config();
        if (device_data == NULL) {
            continue;
        }

        cJSON *json_data = data_sender_prepare_pms_message(device_data->uid, &pms_data);
        char *payload = cJSON_PrintUnformatted(json_data);
        
        data_sender_publish_message(POLLUTION_TELEMETRY_TEMPLATE_TOPIC, device_data->uid, payload);
        
        cJSON_Delete(json_data);
        free(payload);
    }

    vTaskDelete(NULL);
}

bool data_sender_init() {

    mqtt_pms_data_output_queue = xQueueCreate(5, sizeof(pm_data_t));

    bool is_queue_created = mqtt_pms_data_output_queue != NULL;

    if (is_queue_created) {
        xTaskCreate(pms_data_sender_task, "pms data sender task", 4096, NULL, 5, NULL);
    }

    return is_queue_created;
}

bool data_sender_provision_device() {

    device_data_t *device_data = device_helper_get_device_config();
    if (device_data == NULL) {
        return false;
    }

    cJSON *json_data = data_sender_prepare_provisioning_message(device_data->uid);
    char *payload = cJSON_PrintUnformatted(json_data);

    bool result = data_sender_publish_message(PROVISIONING_TEMPLATE_TOPIC, device_data->uid, payload);

    cJSON_Delete(json_data);
    free(payload);

    return result;
}

bool data_sender_enqueue_pms_data(pm_data_t *data) {

    if (mqtt_pms_data_output_queue == NULL) {
        return false;
    }
    
    int result = xQueueGenericSend(mqtt_pms_data_output_queue, data, 0, queueSEND_TO_BACK);
    return result == pdTRUE;
}

bool data_sender_send_temperature_data(float data) {
    
    device_data_t *device_data = device_helper_get_device_config();
    if (device_data == NULL) {
        return false;
    }

    cJSON *json_data = data_sender_prepare_temperature_message(device_data->uid, &data);
    char *payload = cJSON_PrintUnformatted(json_data);

    bool result = data_sender_publish_message(TEMPERATURE_TELEMETRY_TEMPLATE_TOPIC, device_data->uid, payload);
    
    cJSON_Delete(json_data);
    free(payload);

    return result;
}

bool data_sender_send_humidity_data(float data) {

    device_data_t *device_data = device_helper_get_device_config();
    if (device_data == NULL) {
        return false;
    }

    cJSON *json_data = data_sender_prepare_humidity_message(device_data->uid, &data);
    char *payload = cJSON_PrintUnformatted(json_data);

    bool result = data_sender_publish_message(HUMIDITY_TELEMETRY_TEMPLATE_TOPIC, device_data->uid, payload);
    
    cJSON_Delete(json_data);
    free(payload);

    return result;
}
