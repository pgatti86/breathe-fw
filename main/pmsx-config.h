#ifndef PMSX_CONF_INCLUDE_PMSX_CONF_H_
#define PMSX_CONF_INCLUDE_PMSX_CONF_H_

#define UART_PORT 2
#define SET_GPIO 18
#define RESET_GPIO 19
#define TX_GPIO 4
#define RX_GPIO 5

pmsx003_config_t pms_conf = {
    .sensor_id = 1,
    .uart_port = UART_PORT,
    .indoor = false,
    .enabled = true,
    .periodic = true,
    .periodic_sec_interval = 300, // every five minutes
    .set_pin = SET_GPIO,
    .reset_pin = RESET_GPIO,
    .uart_tx_pin = TX_GPIO,
    .uart_rx_pin = RX_GPIO,
};

#endif