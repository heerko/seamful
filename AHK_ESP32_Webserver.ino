#include <LittleFS.h>
#include <DNSServer.h>
#include "WebServerUtils.h"

char g_ssid[32] = "ESP_AP";  // set AP SSID. Can be overwritten by creating a file on LittleFS with extension .ssid

// Do not touch the following variables
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;
const IPAddress localIP(4, 3, 2, 1);
const IPAddress gatewayIP(4, 3, 2, 1);
const IPAddress subnetMask(255, 255, 255, 0);
const char localIPURL[] = "http://4.3.2.1";

bool T3hasChanged = false;
int threshold = 50;

void T3getTouch() {
  T3hasChanged = true;
}

void customEndPoints() {
  // Berichtenopslag en -response
  server.on("/message", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!LittleFS.exists("/message.txt")) {
      File file = LittleFS.open("/message.txt", "w");
      if (!file) {
        Serial.println("Failed to create messages.txt for writing");
        request->send(500, "text/plain", "Failed to create file for writing");
        return;
      }
      file.close();
    }

    if (request->hasParam("message", true)) {
      const AsyncWebParameter *message = request->getParam("message", true);
      File file = LittleFS.open("/message.txt", "a");
      file.print(message->value() + "\n");
      file.close();
    } else {
      Serial.println("No message param. Nothing to add.");
    }

    String response = "Messages:\n";
    File file = LittleFS.open("/message.txt", "r");
    while (file.available()) {
      response += file.readStringUntil('\n') + "\n";
    }
    file.close();
    request->send(200, "text/plain", response);
  });
}

void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return;
  }

  touchAttachInterrupt(T3, T3getTouch, threshold);
  listFiles();
  getSSIDFromFS();
  startSoftAccessPoint(g_ssid, NULL, localIP, gatewayIP);
  setUpDNSServer(dnsServer, localIP);
  setUpWebserver(server, localIP);
  customEndPoints();
}

void loop() {
  // if (T3hasChanged) {
  //   T3hasChanged = false;
  //   bool isTouched = touchRead(T3) < threshold;
  //   String message = String("{\"touch_status\":") + (isTouched ? "\"true\"" : "\"false\"") + "}";
  //   ws.textAll(message);
  //   Serial.println(message);
  // }
  dnsServer.processNextRequest();
  ws.cleanupClients();
}