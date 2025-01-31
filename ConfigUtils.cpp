#include "ConfigUtils.h"

ConfigUtils::ConfigUtils() : loaded(false) {}

// Load config from file
bool ConfigUtils::load() {
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("ERROR: Failed to open config file.");
        return false;
    }

    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        Serial.print("ERROR: Failed to parse config file: ");
        Serial.println(error.c_str());
        return false;
    }

    loaded = true;
    return true;
}

// Save config to file
bool ConfigUtils::save() {
    if (!loaded) {
        Serial.println("ERROR: Cannot save config before loading.");
        return false;
    }

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println("ERROR: Failed to open config file for writing.");
        return false;
    }

    serializeJson(doc, configFile);
    configFile.close();
    return true;
}

// Get an integer value, fail if missing
int ConfigUtils::getInt(const char* key) {
    if (!doc.containsKey(key)) {
        Serial.print("ERROR: Missing integer key: ");
        Serial.println(key);
        return -1;  // Return -1 or handle error differently
    }
    return doc[key].as<int>();
}

// Get a string value, fail if missing
String ConfigUtils::getString(const char* key) {
    if (!doc.containsKey(key)) {
        Serial.print("ERROR: Missing string key: ");
        Serial.println(key);
        return "";
    }
    return String(doc[key].as<const char*>());
}

// Get an array, fail if missing
JsonArray ConfigUtils::getArray(const char* key) {
    if (!doc.containsKey(key)) {
        Serial.print("ERROR: Missing array key: ");
        Serial.println(key);
        return JsonArray();
    }
    return doc[key].as<JsonArray>();
}

// Set integer value
void ConfigUtils::set(const char* key, int value) {
    doc[key] = value;
}

// Set string value
void ConfigUtils::set(const char* key, const String& value) {
    doc[key] = value;
}