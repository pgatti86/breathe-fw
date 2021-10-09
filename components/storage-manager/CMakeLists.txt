set(srcs "storage-manager.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include" REQUIRES nvs_flash)
