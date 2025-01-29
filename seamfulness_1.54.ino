#include <LittleFS.h>
#include <DNSServer.h>
#include "WebServerUtils.h"
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "ConfigClass.h"

// E-Ink Display Configuration
#define EPD_CS 5
#define EPD_DC 17
#define EPD_RST 16
#define EPD_BUSY 4

GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(GxEPD2_213_BN(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

char g_ssid[32] = "ESP_AP";  // set AP SSID. Can be overwritten by creating a file on LittleFS with extension .ssid

// Do not touch the following variables
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;

const IPAddress localIP(4, 3, 2, 1);
const IPAddress gatewayIP(4, 3, 2, 1);
const IPAddress subnetMask(255, 255, 255, 0);
const char localIPURL[] = "http://4.3.2.1";

Config config;
  display.setFullWindow();
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);  // Tekstkleur
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);  // Achtergrondkleur
    display.setCursor(10, 30);        // Positie van de tekst
    display.print(text);
  } while (display.nextPage());
}

void saveMessageToFile(const String &message) {
  if (!LittleFS.exists("/message.txt")) {
    File file = LittleFS.open("/message.txt", "w");
    if (!file) {
      Serial.println("Failed to create messages.txt for writing");
      return;
    }
    file.close();
  }

  File file = LittleFS.open("/message.txt", "a");
  if (file) {
    file.print(message + "\n");
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

void handleMessageEndpoint(AsyncWebServerRequest *request) {
  String response = "ok";

  if (request->hasParam("message", true)) {
    const AsyncWebParameter *message = request->getParam("message", true);
    saveMessageToFile(message->value());
  } else {
    Serial.println("No message param. Nothing to add.");
    response = "Something went wrong. Sorry.";
  }

  // String response = getMessagesFromFile();
  
  request->send(200, "text/plain", response);
}

void handleMessagesTxtEndpoint(AsyncWebServerRequest *request) {
  if (LittleFS.exists("/message.txt")) {
    request->send(LittleFS, "/message.txt", "text/plain");
  } else {
    request->send(404, "text/plain", "File not found.");
  }
}

void handleUpdateDisplayEndpoint(AsyncWebServerRequest *request) {
  if (request->hasParam("text", true)) {
    String text = request->getParam("text", true)->value();
    setDisplayText(text);
    saveMessageToFile(text);
    request->send(200, "text/plain", "Display updated with: " + text);
  } else {
    request->send(400, "text/plain", "No text provided!");
  }
}

void customEndPoints() {
  server.on("/message", HTTP_POST, handleMessageEndpoint);
  server.on("/messages.txt", HTTP_GET, handleMessagesTxtEndpoint);
  server.on("/update-display", HTTP_POST, handleUpdateDisplayEndpoint);
}


  String wifiData = "WIFI:S:" + config.ssid + ";T:nopass;;";
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print("Hello World!");
  } while (display.nextPage());
}


void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return;
  }

  display.init(115200);
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  
  config.load();  // Load configuration

  if (config.role == "ap") {
  startSoftAccessPoint(g_ssid, NULL, localIP, gatewayIP);
  } else {
    Serial.println("Starting WiFi in STA Mode");
    WiFi.mode(WIFI_STA);
  }
  setUpDNSServer(dnsServer, localIP);
  setUpWebserver(server, localIP);
  customEndPoints();
  if (config.role == "AP") {
    showQRCode();
  }
}

void loop() {
  dnsServer.processNextRequest();
  ws.cleanupClients();
}