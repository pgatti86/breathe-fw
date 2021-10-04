#ifndef WIFI_MANAGER_INCLUDE_WIFI_MANAGER_H_
#define WIFI_MANAGER_INCLUDE_WIFI_MANAGER_H_

#include <stdbool.h>
#include <stdio.h>

uint32_t wifi_manager_init(void);

bool wifi_manager_is_connected();

void wifi_manager_deinit(void);

#endif