#include "gpio-manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define DEBOUNCE_TIME 500

#define LONG_CLICK_THRESHOLD 3000

static const char *TAG = "gpio-manager";

typedef struct {
    uint64_t last_interrupt_timestamp;
    pad_conf_t *config;
} pad_data_t;

static pad_data_t pads_data[GPIO_NUM_MAX];

static xQueueHandle gpio_event_queue = NULL;

static uint64_t get_milliseconds_from_boot() {
    return esp_timer_get_time() / 1000;
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_event_queue, &gpio_num, NULL);
}

static void gpio_event_task(void* arg) {
    
    uint8_t io_num;
    int64_t now = 0;
    
    while (true) {

        if (xQueueReceive(gpio_event_queue, &io_num, portMAX_DELAY)) {
            
            pad_data_t *pad_data = &pads_data[io_num];
            now = get_milliseconds_from_boot();

            uint64_t elapsed_time = now - pad_data->last_interrupt_timestamp;
            if (elapsed_time > DEBOUNCE_TIME) {
                ESP_LOGI(TAG, "gpio %d interrupt", io_num);
                uint8_t registered_io_level = pad_data->config->interrput_mode == GPIO_INTERRUPT_FALLING ? 0 : 1;
                while (gpio_get_level(io_num) == registered_io_level) {
                    vTaskDelay(200 / portTICK_RATE_MS);
                }
        
                if (pad_data->config->callback != NULL) {
                    uint64_t press_time = get_milliseconds_from_boot() - now;
                    if (press_time >= LONG_CLICK_THRESHOLD) {
                        pad_data->config->callback(io_num, GPIO_LONG_CLICK);
                    } else {
                        pad_data->config->callback(io_num, GPIO_CLICK);
                    }   
                }
        
                pads_data->last_interrupt_timestamp = get_milliseconds_from_boot();
            }
        }
    }
}

uint32_t gpio_manager_init() {
    
    gpio_event_queue = xQueueCreate(5, sizeof(uint32_t));

    esp_err_t err = gpio_event_queue == NULL ? ESP_FAIL : ESP_OK;
    err += gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    if (err == ESP_OK) {
        xTaskCreate(gpio_event_task, "gpio event task", 2048, NULL, 5, NULL);
    }

    return err;
}

uint32_t gpio_manager_configure_pad(pad_conf_t *conf) {

    gpio_pad_select_gpio(conf->gpio_number);

    gpio_mode_t mode = conf->direction == GPIO_INPUT ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT;
    esp_err_t ret = gpio_set_direction(conf->gpio_number, mode);

    gpio_pull_mode_t pull_mode = GPIO_FLOATING;
    if (conf->pull_mode != GPIO_PULL_NONE) {
        pull_mode = conf->pull_mode  == GPIO_PULL_UP ? GPIO_PULLUP_ONLY : GPIO_PULLDOWN_ONLY;
    }
    ret += gpio_set_pull_mode(conf->gpio_number, pull_mode);

    gpio_int_type_t interrupt_mode = GPIO_INTR_DISABLE;
    if (conf->interrput_mode != GPIO_INTERRUPT_NONE) {
        interrupt_mode = conf->interrput_mode  == GPIO_INTERRUPT_FALLING ? GPIO_INTR_NEGEDGE : GPIO_INTR_POSEDGE;
    }
    ret += gpio_set_intr_type(conf->gpio_number, interrupt_mode);

    if (interrupt_mode != GPIO_INTR_DISABLE) {
        uint32_t pad_number = conf->gpio_number;
        ret += gpio_isr_handler_add(conf->gpio_number, gpio_isr_handler, (void*) pad_number);
    }

    if (ret == ESP_OK) {
        pad_data_t pad_data = {
            .last_interrupt_timestamp = 0,
            .config = conf
        };
        pads_data[conf->gpio_number] = pad_data;
    }

    return ret;
}