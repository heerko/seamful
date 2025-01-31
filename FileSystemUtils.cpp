#include "FileSystemUtils.h"

bool initFileSystem() {
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return false;
  }
  return true;
}

void saveMessageToFile(const String &message) {
  if (!LittleFS.exists("/messages.txt")) {
    File file = LittleFS.open("/messages.txt", "w");
    if (!file) {
      Serial.println("Failed to create messages.txt for writing");
      return;
    }
    file.close();
  }

  File file = LittleFS.open("/messages.txt", "a");
  if (file) {
    file.println(message);
    file.close();
  } else {
    Serial.println("Failed to open messages.txt for appending");
  }
}
std::vector<String> getMessagesFromFile() {
  File file = LittleFS.open("/messages.txt", "r");
  if (!file) {
    Serial.println("Failed to open messages.txt");
    return {};
  }

  std::vector<String> words;
  while (file.available()) {
    words.push_back(file.readStringUntil('\n'));
  }
  file.close();

  return words;
}

void printAllMessages() {
  std::vector<String> words = getMessagesFromFile();
  if (!words.empty()) {
    for (int i = 0; i < words.size(); i++) {
      Serial.println(words[i]);
    }
  }
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