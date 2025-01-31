#ifndef FILE_SYSTEM_UTILS_H
#define FILE_SYSTEM_UTILS_H

#include <Arduino.h>
#include <LittleFS.h>

bool initFileSystem();
void saveMessageToFile(const String &message);
std::vector<String> getMessagesFromFile();
void printAllMessages();
void listFiles();

#endif