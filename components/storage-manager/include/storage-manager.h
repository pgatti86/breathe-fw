#ifndef STORAGE_MANAGER_INCLUDE_STORAGE_MANAGER_H_
#define STORAGE_MANAGER_INCLUDE_STORAGE_MANAGER_H_

#include <stdbool.h>

bool storage_manager_init();

char *storage_manager_read_file(char *file_path);

bool storage_manager_get_prefs_bool_value(char *key, bool def_value);

bool storage_manager_set_prefs_bool_value(char *key, bool value);

#endif