#ifndef DEVICE_HELPER_INCLUDE_DEVICE_HELPER_H_
#define DEVICE_HELPER_INCLUDE_DEVICE_HELPER_H_

#include <stdbool.h>
#include "app-models.h"

device_data_t* device_helper_get_device_config();

bool device_helper_is_enrollment_completed();

bool device_helper_set_enrollment_status(bool completed);

#endif