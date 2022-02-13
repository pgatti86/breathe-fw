# Breathe

Breathe is an open source project for air quality monitoring.
To detect air pollution this project uses a pms5003 sensor; detected data will be sent via mqtt to a cloud service for further analysis and storage.
Temperature and humidity will be recorded too using a DHT22 sensor.

The project is based on Espressif IDF v4.1  
Follow this link to configure your environment: [esp idf v4.1](https://docs.espressif.com/projects/esp-idf/en/v4.1/get-started/index.html)

This repo depends on a library of mine as git submodule:

- [pms5003 library](https://github.com/pgatti86/idf-pms5003)

Refer to this library readme file for sensor pinout and connection.

To clone this project use **git clone --recursive https://github.com/pgatti86/breathe-fw.git**

## Configurations

Before flashing the app you will need to configure the device with **idf.py menuconfig**

### App settings

Enter "Breathe config" menu to configure the application (defaults values applies)

- BROKER_URL: defaults to mqtts://breathe.gatti.dev:8883

- SNTP_SERVER: defaults to "pool.ntp.org"

- DHT_GPIO: gpio connected to DHT22 sensor, defaults to 21

- FW_UPDATE_URL: firmware OTA URL

### Partition Table

The app uses a custom partition table defined in partitions.csv file:

In "Partition table" menù select "Partition Table" sub-menù. Check the "custom partition table CSV" option.
As file name use partitions.csv

### Flash size

You also need to change the embedded flash size: In "Serial flasher" menù enter "Flash size" sub-menù and select the memory size that match your module. The minimum 
flash size required is 4MB.

### OTA

You must allow firmware OTA update to run using http instead of https.
In Component config -> ESP HTTPS OTA, enable allow HTTP for OTA so the URI without TLS is accepted

Save and exit menuconfig.

### Device UID and certificates

Download the device configuration bundle at the following URL:
Once downloaded unzip and copy config.json file into device_config folder.

Look for device_config folder in project, it contains a config.json sample file.
At flashing time this file will be automatically copied into device flash memory.

MQTT connection require clients to use ssl certificate files.
You need the certificate and key files along with the CA file.

- ca.pem
- client.pem
- client.key

Copy certificate files downloaded in previous step into device_certs folder

At flashing time those files will be automatically copied into device flash memory.

**NB: don't change those file names**

## Enrollment

This device use TI smart-config.
To connect the device to your WiFi network you must use one of those app:

- [Android](https://play.google.com/store/apps/details?id=com.dparts.esptouch&hl=it&gl=US)
- [Android](https://play.google.com/store/apps/details?id=com.khoazero123.iot_esptouch_demo&hl=it&gl=US)
- [iOS](https://apps.apple.com/us/app/espressif-esptouch/id1071176700)

I will soon provide an app for device enroll and data visualization

## Reset

You can reset the device by pressing and holding the esp32 BOOT button for 3s.
If your board does't have such button you can wire a button to any GPIO. 
Configure the selected GPIO as show in main.c file.
After reset you need to reconfigure the WiFi as detailed in **Enrollment** section.
Certificate and configuration file are not deleted. Only WiFi settings will be cleared.