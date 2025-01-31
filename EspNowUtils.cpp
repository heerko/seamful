#include "ESPNowUtils.h"

esp_now_peer_info_t client;

void InitESPNow() {
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  } else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}

void initBroadcastClients() {
  memset(&client, 0, sizeof(client));
  memset(client.peer_addr, 0xFF, 6);
  client.channel = WiFi.channel();
  client.encrypt = false;

  if (!esp_now_is_peer_exist(client.peer_addr)) {
    esp_err_t result = esp_now_add_peer(&client);
    Serial.println(result == ESP_OK ? "Broadcast peer added successfully" : "Failed to add broadcast peer");
  } else {
    Serial.println("Broadcast peer already exists");
  }
}

void sendData(const String &message) {
  // prepend the message with the topicIndex and a "|"
  String msg = (String)topicIndex + "|" + message;
  esp_err_t result = esp_now_send(client.peer_addr, (uint8_t *)msg.c_str(), msg.length() + 1);
  Serial.print("Send esp-now message: ");
  Serial.print( msg );
  Serial.print(" Status: ");
  switch (result) {
    case ESP_OK: Serial.println("Success"); break;
    case ESP_ERR_ESPNOW_NOT_INIT: Serial.println("ESPNOW not Init."); break;
    case ESP_ERR_ESPNOW_ARG: Serial.println("Invalid Argument"); break;
    case ESP_ERR_ESPNOW_INTERNAL: Serial.println("Internal Error"); break;
    case ESP_ERR_ESPNOW_NO_MEM: Serial.println("ESP_ERR_ESPNOW_NO_MEM"); break;
    case ESP_ERR_ESPNOW_NOT_FOUND: Serial.println("Peer not found."); break;
    default: Serial.println("Unknown error");
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.printf("Last Packet Sent to: %s - Status: %s\n", macStr, status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void OnDataRecv(const esp_now_recv_info *info, const uint8_t *data, int data_len) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             info->src_addr[0], info->src_addr[1], info->src_addr[2],
             info->src_addr[3], info->src_addr[4], info->src_addr[5]);
    Serial.printf("Received message from: %s\n", macStr);

    if (data_len <= 0) {
        Serial.println("Warning: Received empty message.");
        return;
    }

    char receivedMessage[data_len + 1];
    memcpy(receivedMessage, data, data_len);
    receivedMessage[data_len] = '\0';

    Serial.printf("Received message: %s\n", receivedMessage);

    recievedWord = String(receivedMessage);
    newWordReceived = true;
}