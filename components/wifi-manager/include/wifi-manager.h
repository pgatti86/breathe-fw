#ifndef WIFI_MANAGER_INCLUDE_WIFI_MANAGER_H_
#define WIFI_MANAGER_INCLUDE_WIFI_MANAGER_H_

#include "esp_system.h"

esp_err_t wifi_manager_init(void);

void wifi_manager_deinit(void);

#endif