#ifndef GPIO_MANAGER_INCLUDE_GPIO_MANAGER_H_
#define GPIO_MANAGER_INCLUDE_GPIO_MANAGER_H_

#include <stdio.h>

typedef enum {
    GPIO_INPUT,
    GPIO_OUTPUT
} pad_direction_t;

typedef enum {
    GPIO_INTERRUPT_NONE,
    GPIO_INTERRUPT_FALLING,
    GPIO_INTERRUPT_RISING
} pad_interrupt_t;

typedef enum {
    GPIO_PULL_NONE,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN
} pad_pull_mode_t;

typedef enum {
    GPIO_CLICK,
    GPIO_LONG_CLICK
} pad_event_t;

typedef void (*gpio_callback_f)(uint8_t, pad_event_t);

typedef struct {
    uint8_t gpio_number;
    pad_direction_t direction;
    pad_pull_mode_t pull_mode;
    pad_interrupt_t interrput_mode;
    gpio_callback_f callback;
} pad_conf_t;

uint32_t gpio_manager_init();

uint32_t gpio_manager_configure_pad(pad_conf_t *conf);

#endif