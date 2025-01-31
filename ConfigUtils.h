#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

class ConfigUtils {
private:
    StaticJsonDocument<512> doc;
    bool loaded;

public:
    ConfigUtils();
    bool load();
    bool save();
    
    int getInt(const char* key);
    String getString(const char* key);
    JsonArray getArray(const char* key);

    void set(const char* key, int value);
    void set(const char* key, const String& value);
};

#endif // CONFIG_UTILS_H