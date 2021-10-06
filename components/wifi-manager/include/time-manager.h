#ifndef TIME_MANAGER_INCLUDE_TIME_MANAGER_H_
#define TIME_MANAGER_INCLUDE_TIME_MANAGER_H_

#include <stdbool.h>

#define ISO_DATE_LENGTH 18

void time_manager_sync_time();

bool time_manager_is_time_synched();

void time_manager_format_time(char *out, int length);

#endif