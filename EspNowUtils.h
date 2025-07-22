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

/* This is somewhat tricky. Sending a struct over esp-now
We need to respect the alignment. */
typedef struct __attribute__((packed)) {
    int topicIndex;     // 4 bytes (aligned naturally)
    char message[128];  // 128 bytes (aligned naturally)
    bool isRequest;     // 1 byte
    bool isBroadcast;   // 1 byte
} ESPNowMessage;

static_assert(sizeof(ESPNowMessage) == 134, "Struct size mismatch!");

void InitESPNow();
void initBroadcastClients();
void sendData(const String &message, bool isBroadcast);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int data_len);
void requestMessage();

#endif