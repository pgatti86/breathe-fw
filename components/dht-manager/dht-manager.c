/*
 *  Note:
 *  A suitable pull-up resistor should be connected to the selected GPIO line
 *
 *  __           ______          _______                              ___________________________
 *    \    A    /      \   C    /       \   DHT duration_data_low    /                           \
 *     \_______/   B    \______/    D    \__________________________/   DHT duration_data_high    \__
 *
 *
 *  Initializing communications with the DHT requires four 'phases' as follows:
 *
 *  Phase A - MCU pulls signal low for at least 18000 us
 *  Phase B - MCU allows signal to float back up and waits 20-40us for DHT to pull it low
 *  Phase C - DHT pulls signal low for ~80us
 *  Phase D - DHT lets signal float back up for ~80us
 *
 *  After this, the DHT transmits its first bit by holding the signal low for 50us
 *  and then letting it float back high for a period of time that depends on the data bit.
 *  duration_data_high is shorter than 50us for a logic '0' and longer than 50us for logic '1'.
 *
 *  There are a total of 40 data bits transmitted sequentially. These bits are read into a byte array
 *  of length 5.  The first and third bytes are humidity (%) and temperature (C), respectively.  Bytes 2 and 4
 *  are zero-filled and the fifth is a checksum such that:
 *
 *  byte_5 == (byte_1 + byte_2 + byte_3 + btye_4) & 0xFF
 *
 */

#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include <esp_log.h>

#include "dht-manager.h"

static const char *TAG = "DHTxx";

// DHT timer precision in microseconds
#define DHT_TIMER_INTERVAL 2
#define DHT_DATA_BITS 40
#define DHT_DATA_BYTES (DHT_DATA_BITS / 8)
#define UPDATE_TASK_DELAY 30000

#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)

#define CHECK_LOGE(x, msg, ...) do { \
        esp_err_t __; \
        if ((__ = x) != ESP_OK) { \
            PORT_EXIT_CRITICAL; \
            ESP_LOGE(TAG, msg, ## __VA_ARGS__); \
            return __; \
        } \
    } while (0)

static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#define PORT_ENTER_CRITICAL portENTER_CRITICAL(&mux)
#define PORT_EXIT_CRITICAL portEXIT_CRITICAL(&mux)

static TaskHandle_t dht_task_handle = NULL;
static float humidity = 0.;
static float temperature = 0.;

static esp_err_t dht_manager_read_data(dht_sensor_type_t sensor_type, gpio_num_t pin, int16_t *humidity, int16_t *temperature);

static esp_err_t dht_manager_read_float_data(dht_sensor_type_t sensor_type, gpio_num_t pin, float *humidity, float *temperature);

static esp_err_t dht_await_pin_state(gpio_num_t pin, uint32_t timeout, int expected_pin_state, uint32_t *duration);

static inline int16_t dht_convert_data(dht_sensor_type_t sensor_type, uint8_t msb, uint8_t lsb);

/**
 * Request data from DHT and read raw bit stream.
 * The function call should be protected from task switching.
 * Return false if error occurred.
 */
static inline esp_err_t dht_fetch_data(dht_sensor_type_t sensor_type, 
                                       gpio_num_t pin, 
                                       uint8_t data[DHT_DATA_BYTES])
{
    uint32_t low_duration;
    uint32_t high_duration;

    // Phase 'A' pulling signal low to initiate read sequence
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 0);
    ets_delay_us(sensor_type == DHT_TYPE_SI7021 ? 500 : 20000);
    gpio_set_level(pin, 1);

    // Step through Phase 'B', 40us
    CHECK_LOGE(dht_await_pin_state(pin, 40, 0, NULL),
            "Initialization error, problem in phase 'B'");
    // Step through Phase 'C', 88us
    CHECK_LOGE(dht_await_pin_state(pin, 88, 1, NULL),
            "Initialization error, problem in phase 'C'");
    // Step through Phase 'D', 88us
    CHECK_LOGE(dht_await_pin_state(pin, 88, 0, NULL),
            "Initialization error, problem in phase 'D'");

    // Read in each of the 40 bits of data...
    for (int i = 0; i < DHT_DATA_BITS; i++)
    {
        CHECK_LOGE(dht_await_pin_state(pin, 65, 1, &low_duration),
                "LOW bit timeout");
        CHECK_LOGE(dht_await_pin_state(pin, 75, 0, &high_duration),
                "HIGH bit timeout");

        uint8_t b = i / 8;
        uint8_t m = i % 8;
        if (!m)
            data[b] = 0;

        data[b] |= (high_duration > low_duration) << (7 - m);
    }

    return ESP_OK;
}

/**
 * @brief Read data from sensor on specified pin.
 * Humidity and temperature are returned as integers.
 * For example: humidity=625 is 62.5 %
 *              temperature=24.4 is 24.4 degrees Celsius
 *
 * @param[in] sensor_type DHT11 or DHT22
 * @param[in] pin GPIO pin connected to sensor OUT
 * @param[out] humidity Humidity, percents * 10
 * @param[out] temperature Temperature, degrees Celsius * 10
 * @return `ESP_OK` on success
 */
static esp_err_t dht_manager_read_data(dht_sensor_type_t sensor_type, 
                                gpio_num_t pin, 
                                int16_t *humidity, 
                                int16_t *temperature) {

    CHECK_ARG(humidity && temperature);

    uint8_t data[DHT_DATA_BYTES] = { 0 };

    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 1);

    PORT_ENTER_CRITICAL;
    esp_err_t result = dht_fetch_data(sensor_type, pin, data);
    if(result == ESP_OK){
        PORT_EXIT_CRITICAL;
    }

    /* restore GPIO direction because, after calling dht_fetch_data(), the
     * GPIO direction mode changes */
    gpio_set_direction(pin, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(pin, 1);

    if (result != ESP_OK)
        return result;

    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
    {
        ESP_LOGE(TAG, "Checksum failed, invalid data received from sensor");
        return ESP_ERR_INVALID_CRC;
    }

    *humidity = dht_convert_data(sensor_type, data[0], data[1]);
    *temperature = dht_convert_data(sensor_type, data[2], data[3]);

    ESP_LOGD(TAG, "Sensor data: humidity=%d, temp=%d", *humidity, *temperature);

    return ESP_OK;
}

/**
 * @brief Read data from sensor on specified pin.
 * Humidity and temperature are returned as floats.
 *
 * @param[in] sensor_type DHT11 or DHT22
 * @param[in] pin GPIO pin connected to sensor OUT
 * @param[out] humidity Humidity, percents
 * @param[out] temperature Temperature, degrees Celsius
 * @return `ESP_OK` on success
 */
static esp_err_t dht_manager_read_float_data(dht_sensor_type_t sensor_type, 
                                      gpio_num_t pin, 
                                      float *humidity, 
                                      float *temperature) {
    
    CHECK_ARG(humidity && temperature);

    int16_t i_humidity, i_temp;

    esp_err_t res = dht_manager_read_data(sensor_type, pin, &i_humidity, &i_temp);
    if (res != ESP_OK)
        return res;

    *humidity = i_humidity / 10.0;
    *temperature = i_temp / 10.0;

    return ESP_OK;
}

/**
 * Wait specified time for pin to go to a specified state.
 * If timeout is reached and pin doesn't go to a requested state
 * false is returned.
 * The elapsed time is returned in pointer 'duration' if it is not NULL.
 */
static esp_err_t dht_await_pin_state(gpio_num_t pin, uint32_t timeout,
       int expected_pin_state, uint32_t *duration)
{
    /* XXX dht_await_pin_state() should save pin direction and restore
     * the direction before return. however, the SDK does not provide
     * gpio_get_direction().
     */
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    for (uint32_t i = 0; i < timeout; i += DHT_TIMER_INTERVAL)
    {
        // need to wait at least a single interval to prevent reading a jitter
        ets_delay_us(DHT_TIMER_INTERVAL);
        if (gpio_get_level(pin) == expected_pin_state)
        {
            if (duration)
                *duration = i;
            return ESP_OK;
        }
    }

    return ESP_ERR_TIMEOUT;
}

/**
 * Pack two data bytes into single value and take into account sign bit.
 */
static inline int16_t dht_convert_data(dht_sensor_type_t sensor_type, uint8_t msb, uint8_t lsb)
{
    int16_t data;

    if (sensor_type == DHT_TYPE_DHT11)
    {
        data = msb * 10;
    }
    else
    {
        data = msb & 0x7F;
        data <<= 8;
        data |= lsb;
        if (msb & BIT(7))
            data = -data;       // convert it to negative
    }

    return data;
}

/**
 * Updates internal values
 */
static void dht_task(void *args) {

   while (true) {
       dht_manager_read_float_data(DHT_TYPE_AM2301, CONFIG_DHT_GPIO, &humidity, &temperature);
       vTaskDelay(UPDATE_TASK_DELAY / portTICK_RATE_MS);
   }
}

void dht_manager_start_update_task() {
    
    if (dht_task_handle) {
        dht_manager_stop_update_task();
    }

    ESP_LOGI(TAG, "Starting dht task....");
    xTaskCreate(&dht_task, "dht_task", 1024, NULL, 3, &dht_task_handle);
}

void dht_manager_stop_update_task() {

    ESP_LOGI(TAG, "Stopping dht task....");
    vTaskDelete(dht_task_handle);
}

void dht_manager_get_last_red_values(float *t, float *h) {
    *t = temperature;
    *h = humidity;
}