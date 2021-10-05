#ifndef DATA_SENDER_INCLUDE_DATA_SENDER_H_
#define DATA_SENDER_INCLUDE_DATA_SENDER_H_

#include <stdbool.h>
#include "pmsx-data.h"

bool data_sender_send_pms_data(char *device_id, pm_data_t *data);

#endif