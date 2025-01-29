#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <Arduino.h> // Required for String

class ConfigUtils {
public:
    String role;
    String topic;
    String ssid;
    int channel;

    ConfigUtils();        // Constructor declaration
    void load();     // Method declarations
    void save();
};

#endif // CONFIG_UTILS_H