#ifndef DEVICE_HELPER_INCLUDE_DEVICE_HELPER_H_
#define DEVICE_HELPER_INCLUDE_DEVICE_HELPER_H_

#include <stdbool.h>
#include "app-models.h"

typedef enum { COMPLETED, INCOMPLETED} enrollment_status_t;

device_data_t* device_helper_get_device_config();

bool device_helper_is_enrollment_completed();

bool device_helper_set_enrollment_status(enrollment_status_t status);

char *device_helper_get_ca_cert();

char *device_helper_get_device_cert();

char *device_helper_get_device_key();

#endif