#ifndef WIFI_MANAGER_INCLUDE_WIFI_EVENTS_H_
#define WIFI_MANAGER_INCLUDE_WIFI_EVENTS_H_

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(WIFI_MANAGER_EVENTS);

enum {                                      
    WIFI_EVENT_CONNECTED,
    WIFI_EVENT_DISCONNECTED
};

#endif