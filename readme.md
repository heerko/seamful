Implementation of a softAP on ESP32. 

Features:
- Websockets
- Serves files off LittleFS
- Optionally set SSID via file on LittleFS
- creates esp-now network withother displays nearby 

Configuration
=============
data/config.json
```.json
{
    "is_ap": 0,
    "topic": 2,
    "channel" : 1,
    "topics": [
        "closed source",
        "open source",
        "network imaginaries"
    ],
    "ssids": [
        "･ﾟﾟ･｡Smooth ride?*･",
        "｡Bumpy tool adventures!｡",
        "˚Collective tool ecologies˚"
    ],
    "info_mode": "SSID",
    "info_interval": 5,
    "min_interval": 10000,
    "max_interval": 20000
}
```
- "is_ap": [bool] This module functions as a access point
- "topic": [int] Index of the topic to display
- "channel": [int] Wifi channel
- "topics": [
        "closed source",
        "open source",
        "network imaginaries"
    ], [string array] List of topics. No longer used, only as labels internally.
- "ssids": [
        "･ﾟﾟ･｡Smooth ride?*･",
        "｡Bumpy tool adventures!｡",
        "˚Collective tool ecologies˚"
    ], [string array] List of SSIDs, corresponding to the topics
- "info_mode": ["SSID" or "QR", anything else disables
- "info_interval": [int] after how many slides should info be shown
- "min_interval": [int] minimun random interval in ms
- "max_interval": [int] maximum random interval in ms


Usage
=====
Tested on ESP32 dev kit with ESP-WROOM-32. 


Note: Do not install Esp32 boards with version above 3.0.6, 
there seems to be a incompatibility between the newer versions 
of the ESP32 boards package and Asyncwebserver.
For it seems to work well with ESP32 boards v3.0.6 and ESPAsyncWebServer 3.4.5

