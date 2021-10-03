#ifndef MQTT_MANAGER_INCLUDE_MQTT_MANAGER_H_
#define MQTT_MANAGER_INCLUDE_MQTT_MANAGER_H_

#include "esp_system.h"

esp_err_t mqtt_manager_init();

esp_err_t mqtt_manager_connect();

bool mqtt_manager_is_connected();

bool mqtt_manager_publish(char *topic, char *json);

esp_err_t mqtt_manager_disconnect();

#endif