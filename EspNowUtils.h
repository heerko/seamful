#ifndef ESP_NOW_UTILS_H
#define ESP_NOW_UTILS_H

#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>

extern esp_now_peer_info_t client;

void InitESPNow();
void initBroadcastClients();
void sendData(const String &message);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int data_len);

#endif