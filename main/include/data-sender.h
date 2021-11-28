#ifndef DATA_SENDER_INCLUDE_DATA_SENDER_H_
#define DATA_SENDER_INCLUDE_DATA_SENDER_H_

#include <stdbool.h>
#include "pmsx-data.h"

bool data_sender_init();

bool data_sender_provision_device();

bool data_sender_enqueue_pms_data(pm_data_t *data);

#endif