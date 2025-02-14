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
  memset(client.peer_addr, 0xFF, 6);  // Broadcast
  client.channel = WiFi.channel();
  client.encrypt = false;

  if (!esp_now_is_peer_exist(client.peer_addr)) {
    esp_err_t result = esp_now_add_peer(&client);
    Serial.println(result == ESP_OK ? "Broadcast peer added successfully" : "Failed to add broadcast peer");
  }

  // Register a direct peer (only needed for the client!)
  if (!is_ap) {  // Only clients need to register the AP as a peer
    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(peer));
    WiFi.macAddress(peer.peer_addr);  // Get client's MAC
    peer.channel = WiFi.channel();
    peer.encrypt = false;
    if (!esp_now_is_peer_exist(peer.peer_addr)) {
      esp_err_t result = esp_now_add_peer(&peer);
      Serial.println(result == ESP_OK ? "Direct AP peer added successfully" : "Failed to add AP peer");
    }
  }
}

void sendData(const String &message) {
  ESPNowMessage data;
  data.topicIndex = topicIndex;
  data.isRequest = false;  // This is a message, not a request
  strncpy(data.message, message.c_str(), sizeof(data.message));

  esp_err_t result = esp_now_send(client.peer_addr, (uint8_t *)&data, sizeof(data));
  
  Serial.print("Send ESP-NOW message: ");
  Serial.print(message);
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
  Serial.println("ESP-NOW MESSAGE RECEIVED!");

  if (data_len <= 0) {
      Serial.println("Received empty message.");
      return;
  }

  Serial.printf("Received raw data (%d bytes): ", data_len);
  for (int i = 0; i < data_len; i++) {
      Serial.printf("%02X ", data[i]);
  }
  Serial.println();

  if (data_len == sizeof(ESPNowMessage)) {
      ESPNowMessage received;
      memcpy(&received, data, sizeof(ESPNowMessage));

      Serial.printf("Received ESP-NOW message (isRequest: %d, topic: %d)\n", received.isRequest, received.topicIndex);

      if (received.isRequest && is_ap && received.topicIndex == topicIndex) {
          String word = getRandomMessage();
          Serial.printf("Sending response for topic %d: %s\n", received.topicIndex, word.c_str());
          sendData(word);  // 🔹 Use `sendData()` instead of manual send
      } 
      else if (!received.isRequest && !is_ap) {
          recievedWord = String(received.message);
          newWordReceived = true;
          Serial.printf("Received message: %s\n", received.message);
      }
  }
}

void requestMessage() {

  if (millis() - lastRequestTime < 1000) return;  // Prevent spamming requests
  lastRequestTime = millis();

  ESPNowMessage request;
  request.topicIndex = topicIndex;
  request.isRequest = true;
  memset(request.message, 0, sizeof(request.message));

  esp_now_send(client.peer_addr, (uint8_t *)&request, sizeof(request));
  Serial.printf("Requested message for topic %d\n", topicIndex);
}