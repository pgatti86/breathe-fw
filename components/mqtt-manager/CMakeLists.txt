set(srcs "mqtt-manager.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include" REQUIRES mqtt)