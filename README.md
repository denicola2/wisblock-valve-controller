## WisBlock Valve Controller
Ball Valve controller based on the WisBlock and RAK4631, using the [WisBlock-API library](https://github.com/beegee-tokyo/WisBlock-API)

# Hardware Used
Here is the list of hardware used, substitutions can be made based on use-case
- [WisBlock Base RAK5005-O](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK5005-O/Datasheet/)
- [WisBlock Core RAK4631](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK4631/Datasheet/)
- [WisBlock IO Expansion Module RAK13003](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK13003/Datasheet/)
- [WisBlock RAKBOX B2 Enclosure](https://docs.rakwireless.com/Product-Categories/Accessories/RAKBox-B2/Datasheet/) (NOTE: The fitting was VERY tight in the B2 enclosure, a larger enclosure would be recommended for future iterations)
- [2x DC 3.3V Relay](https://www.amazon.com/Channel-Optocoupler-Isolated-Control-Arduino/dp/B07XGZSYJV)
- [3/4" Motorized Ball Valve from US Solid](https://www.amazon.com/dp/B06XRJF4JG) / [Datasheet](https://m.media-amazon.com/images/I/81iA78dQJbL.pdf)
- Batteries: 3.7V 2000mAh Lipo Battery, 9V Battery

# Connections Diagram
![Valve Controller](https://user-images.githubusercontent.com/8965585/171214857-dc32ca38-134b-44fb-b66c-85cc1378d578.jpg)

# Setup
- WisBlock Base, Core, and IO Expansion Module per the instructions in the [RAK WisBlock Quickstart Guide](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK4631/Quickstart/)
- If you are using Visual Studio, the following `platformio.ini` can be used to take care of the library requirements:
```
; PlatformIO Project Configuration File
;
;   Build options: build flags, source filter
;   Upload options: custom upload port, speed and extra flags
;   Library options: dependencies, extra library storages
;   Advanced options: extra scripting
;
; Please visit documentation for the other options and examples
; https://docs.platformio.org/page/projectconf.html

[env:wiscore_rak4631]
platform = nordicnrf52
board = wiscore_rak4631
framework = arduino
lib_deps = 
	beegee-tokyo/SX126x-Arduino@^2.0.11
	adafruit/Adafruit MCP23017 Arduino Library@^2.1.0
	beegee-tokyo/WisBlock-API@^1.1.15
; build_flags = -DAPI_DEBUG=1 -DMY_DEBUG=1
```

# Build and Upload
- Before building the code, be sure to modify the LoRaWAN keys in `main.cpp` to be unique to your device
```
uint8_t node_device_eui[8] = {0x00, 0x0D, 0x75, 0xE6, 0x56, 0x4D, 0xC1, 0xF3};
uint8_t node_app_eui[8] = {0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x02, 0x01, 0xE1};
uint8_t node_app_key[16] = {0x2B, 0x84, 0xE0, 0xB0, 0x9B, 0x68, 0xE5, 0xCB, 0x42, 0x17, 0x6F, 0xE7, 0x53, 0xDC, 0xEE, 0x79};
```
- Additionally, check that the LoRaWAN settings within `setup_app()` align with your region
```
g_lorawan_settings.app_port = LORAWAN_APP_PORT;  // Data port to send data
g_lorawan_settings.lora_region = LORAMAC_REGION_US915;  // LoRa region
g_lorawan_settings.tx_power = TX_POWER_0;  // TX power 0 .. 15 (validity depends on Region)
g_lorawan_settings.data_rate = DR_3;  // Data rate 0 .. 15 (validity depends on Region)
g_lorawan_settings.subband_channels = 2; // Subband channel selection 1 .. 9
```
- Use PlatformIO or Arduino IDE to upload to the target

# Verify Functionality
- LoRaWAN CLASS A should auto-join the network provided there is a gateway in range. Check to see if your device has joined the network. 
- Use the [Bluefruit Connect](https://apps.apple.com/us/app/bluefruit-connect/id830125974) app to scan for your device (RAK-VLVC-XXXX) and attempt to connect. By default, the code is set to advertise BLE indefinitely.
- Interact over UART from the app, or via LoRaWAN downlink to the device to do the following:
  1. Check if LoRaWAN join was successful: `AT+JOIN=?`
  2. Check the LiPo battery status: `AT+BAT=?`
  3. Manually open the valve: `AT+VLVS=1`
  4. Manually close the valve: `AT+VLVS=0`
  5. Open the valve for a specified number of seconds: `AT+VLVI=45`
  6. Check the current state of the valve: `AT+VLVS=?`
  7. View more info about WisBlock AT commands [here](https://github.com/beegee-tokyo/WisBlock-API/blob/main/AT-Commands.md)

# Sample screenshots from BLE UART Interactions
<img src="https://user-images.githubusercontent.com/8965585/171219798-5b4922c7-e3e6-4572-bb4c-408c106a84ad.png" height=1024 width=512>

# Project Photo
![image](https://user-images.githubusercontent.com/8965585/171220811-22b14be2-dafe-4aa6-9f13-93ed6002d2fa.png)

# Future Improvements
- Add circuitry or a new RAK module to measure and track the capacity of the 9V battery used to drive the valve motor.
- Add RAK12002 RTC module to allow for the controller to keep a programmable open/close schedule.




 
 

