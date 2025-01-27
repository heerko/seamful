Implementation of a softAP on ESP32. 

Features:
- Websockets
- Serves files off LittleFS
- Optionally set SSID via file on LittleFS

Usage
=====
Tested on ESP32 dev kit with ESP-WROOM-32. 

Note: Do not install Esp32 boards with version above 3.0.6, 
there seems to be a incompatibility between the newer versions 
of the ESP32 boards package and Asyncwebserver.