#ifndef ESP_NOW_UTILS_H
#define ESP_NOW_UTILS_H

#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include "FileSystemUtils.h"

extern esp_now_peer_info_t client;
extern bool newWordReceived;
extern String recievedWord;
extern int topicIndex;
extern int is_ap;
static unsigned long lastRequestTime = 0;

typedef struct __attribute__((packed)) {
    int topicIndex;     // Topic index
    bool isRequest;     // True if it's a request, False if it's a response
    char message[128];  // Message (empty for requests, filled for responses)
} ESPNowMessage;

void InitESPNow();
void initBroadcastClients();
void sendData(const String &message);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int data_len);
void requestMessage();

#endif