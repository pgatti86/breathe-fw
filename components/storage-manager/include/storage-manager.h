#ifndef STORAGE_MANAGER_INCLUDE_STORAGE_MANAGER_H_
#define STORAGE_MANAGER_INCLUDE_STORAGE_MANAGER_H_

#include <stdbool.h>

bool storage_manager_init();

bool storage_manager_set_enrollment_status(bool completed);

bool storage_manager_has_enrollment_done();

#endif