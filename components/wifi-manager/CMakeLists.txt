set(srcs "wifi-manager.c" "time-manager.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include")