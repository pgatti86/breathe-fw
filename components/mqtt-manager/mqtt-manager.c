#include "mqtt-manager.h"

#include "esp_event.h"
#include "mqtt_client.h"
#include "esp_log.h"

static const char *TAG = "mqtt-manager";

static esp_mqtt_client_handle_t client;

static bool is_connected = false;

static bool is_started = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            is_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            is_connected = false;
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

esp_err_t mqtt_manager_init() {

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    return esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

esp_err_t mqtt_manager_connect() {
    esp_err_t status = is_started ? esp_mqtt_client_reconnect(client) : esp_mqtt_client_start(client);
    is_started = true;
    return status;
}

bool mqtt_manager_is_connected() {
    return is_connected;
}

bool mqtt_manager_publish(const char *topic, const char *data) {

    if (!is_connected)
        return false;

    int data_len = 0;
    int qos = 0;
    int retain = 0;
    return esp_mqtt_client_publish(client, topic, data, data_len, qos, retain) > 0;
}

esp_err_t mqtt_manager_disconnect() {

    if (!mqtt_manager_is_connected()) {
        return ESP_OK;
    }

    return esp_mqtt_client_disconnect(client);
}