#ifndef FILE_SYSTEM_UTILS_H
#define FILE_SYSTEM_UTILS_H

#include <Arduino.h>
#include <LittleFS.h>


extern int topicIndex;

bool initFileSystem();
void initializeMessagesFile();
void saveMessageToFile(const String &message);
std::vector<String> getMessagesFromFile();
void trimOldMessages();
String getRandomMessage();
void printAllMessages();
void listFiles();

#endif