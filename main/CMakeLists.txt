idf_component_register(
    SRCS "data-sender.c" "main.c" "device-helper.c"
    INCLUDE_DIRS "include"
)

# Create a SPIFFS image from the contents of the 'device_config' directory
# that fits the partition named 'device'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
spiffs_create_partition_image(device ../device_config FLASH_IN_PROJECT)

# Create a SPIFFS image from the contents of the 'device_certs' directory
# that fits the partition named 'security'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
spiffs_create_partition_image(security ../device_certs FLASH_IN_PROJECT)
