#include "FileSystemUtils.h"

bool initFileSystem() {
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return false;
  }
  return true;
}

void saveMessageToFile(const String &message) {
  if (!LittleFS.exists("/message.txt")) {
    File file = LittleFS.open("/message.txt", "w");
    if (!file) {
      Serial.println("Failed to create message.txt for writing");
      return;
    }
    file.close();
  }

  File file = LittleFS.open("/message.txt", "a");
  if (file) {
    file.println(message);
    file.close();
  } else {
    Serial.println("Failed to open message.txt for appending");
  }
}

String getMessagesFromFile() {
  String response = "Messages:\n";

  if (LittleFS.exists("/message.txt")) {
    File file = LittleFS.open("/message.txt", "r");
    if (file) {
      while (file.available()) {
        response += file.readStringUntil('\n') + "\n";
      }
      file.close();
    } else {
      Serial.println("Failed to open message.txt for reading");
    }
  } else {
    response += "No messages found.\n";
  }

  return response;
}

void listFiles() {
  Serial.println("Listing files in LittleFS:");
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("File: %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }
}