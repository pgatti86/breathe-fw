/**
 * @file dht_manager.h
 * @defgroup dht_manager dht_manager
 * @{
 *
 * ESP-IDF driver for DHT11, AM2301 (DHT21, DHT22, AM2302, AM2321), Itead Si7021
 *
 * Ported from esp-open-rtos
 *
 * Copyright (C) 2016 Jonathan Hartsuiker <https://github.com/jsuiker>\n
 * Copyright (C) 2018 Ruslan V. Uss <https://github.com/UncleRus>\n
 *
 * BSD Licensed as described in the file LICENSE
 *
 * @note A suitable pull-up resistor should be connected to the selected GPIO line
 *
 */
#ifndef __DHT_H__
#define __DHT_H__

#include <driver/gpio.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sensor types
 */
typedef enum {
    DHT_TYPE_DHT11 = 0,   //!< DHT11
    DHT_TYPE_AM2301,      //!< AM2301 (DHT21, DHT22, AM2302, AM2321)
    DHT_TYPE_SI7021       //!< Itead Si7021
} dht_sensor_type_t;

/**
 * Start dht low priority task to update internal values.
 * You can get those internal values with #dht_get_last_red_values method
 */
void dht_manager_start_update_task();

/**
 * Stop dht internal task.
 */
void dht_manager_stop_update_task();

/**
 * Get last red available values
 */
void dht_manager_get_last_red_values(float *temperature, float *humidity);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif  // __DHT_H__