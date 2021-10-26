#ifndef MQTT_MANAGER_INCLUDE_MQTT_MANAGER_H_
#define MQTT_MANAGER_INCLUDE_MQTT_MANAGER_H_

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    char *ca_cert;
    char *device_cert;
    char *device_key;
} mqtt_certificates_t;

typedef void(*load_certs_f)(mqtt_certificates_t*);

uint32_t mqtt_manager_init(load_certs_f f);

uint32_t mqtt_manager_connect();

bool mqtt_manager_is_connected();

bool mqtt_manager_publish(const char *topic, const char *data);

uint32_t mqtt_manager_disconnect();

#endif