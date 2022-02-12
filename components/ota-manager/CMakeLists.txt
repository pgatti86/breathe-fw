set(srcs "ota-manager.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include" REQUIRES esp_https_ota app_update)
