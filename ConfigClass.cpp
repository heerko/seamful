#include "ConfigClass.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// Constructor implementation
Config::Config() : role("ap"), topic("closed"), ssid("set_ssid_in_config_json"), channel(1) {}

// Method to load configuration from file
void Config::load() {
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("Failed to open config file");
        return;
    }

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        Serial.println("Failed to parse config file");
        configFile.close();
        return;
    }

    role = doc["role"] | "ap";
    topic = doc["topic"] | "closed";
    ssid = doc["ssid"] | "set_ssid_in_config_json";
    channel = doc["channel"] | 1;
    configFile.close();

    Serial.println("Config loaded:");
    serializeJson(doc, Serial);
    Serial.println();
}

// Method to save configuration to file
void Config::save() {
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return;
    }

    StaticJsonDocument<256> doc;
    doc["role"] = role;
    doc["topic"] = topic;
    doc["ssid"] = ssid;
    doc["channel"] = channel;

    serializeJson(doc, configFile);
    configFile.close();

    Serial.println("Config saved:");
    serializeJson(doc, Serial);
    Serial.println();
}