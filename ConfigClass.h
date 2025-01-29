#ifndef CONFIG_CLASS_H
#define CONFIG_CLASS_H

#include <Arduino.h> // Required for String

class Config {
public:
    String role;
    String topic;
    String ssid;
    int channel;

    Config();        // Constructor declaration
    void load();     // Method declarations
    void save();
};

#endif // CONFIG_CLASS_H