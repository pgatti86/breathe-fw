# Breathe

Breathe is an open source project for air quality monitoring.  
To detect air pollution this project uses a pms5003 sensor; detected data will be sent via mqtt for further analysis and storage.

The project is based on Espressif IDF v4.1  
Follow this link to configure your environment: [esp idf v4.1](https://docs.espressif.com/projects/esp-idf/en/v4.1/get-started/index.html)

This repo depends on a library of mine as git submodule:

- [pms5003 library](https://github.com/pgatti86/idf-pms5003)

To clone this project use **git clone --recursive https://github.com/pgatti86/breathe-fw.git**

## Configurations

Before flashing the app you will need to configure the device with **idf.py menuconfig**

### App settings

Enter "Breathe config" menu to configure the application (defaults values applies)

- BROKER_URL: defaults to 80.211.97.124

- SNTP_SERVER: defaults to "pool.ntp.org"

### Partition Table

The app uses a custom partition table defined in partitions.csv file:

In "Partition table" menù select "Partition Table" sub-menù. Check the "custom partition table CSV" option.

### Flash size

You also need to change the embedded flash size: In "Serial flasher" menù enter "Flash size" sub-menù and select the memory size that match your module. The minimum size required is 4MB.

Save and exit menuconfig.

### Device UID

Look for device_config folder in project, it contains a config.json file.
Generate a new UUID for your device. It will be used in mqtt communication for device identification.
You can generate a random uid [here](https://www.uuidgenerator.net)

At compilation time this file will be automatically copied to device flash.

**NB: don't change file name**

### Device certificates

MQTT connection require clients to use ssl certificate files.
You need the certificate and key files along with the CA file.

- ca.pem
- client.pem
- client.key

I will soon provide a method for generating certificates.

At compilation time these files will be automatically copied to device flash.

**NB: don't change these files name**

## Enrollment

This device use TI smart-config.
To connect the device to your WiFi network you must use one of those app:

- [Android](https://play.google.com/store/apps/details?id=com.dparts.esptouch&hl=it&gl=US)
- [Android](https://play.google.com/store/apps/details?id=com.khoazero123.iot_esptouch_demo&hl=it&gl=US)
- [iOS](https://apps.apple.com/us/app/espressif-esptouch/id1071176700)

I will soon provide an app for device enroll and recorded data visualization

## Reset

You can reset the device by pressing and holding the esp32 BOOT button for 3s.
If your board does't have such button you can wire a button to any GPIO. 
Configure the selected GPIO as show in main.c file 