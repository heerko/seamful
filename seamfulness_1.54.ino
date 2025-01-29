
#include <LittleFS.h>
#include <DNSServer.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <ArduinoJson.h>
#include <QRCodeGenerator.h>

// parts.
#include "WebServerUtils.h"
#include "FileSystemUtils.h"
#include "ESPNowUtils.h"
#include "ConfigUtils.h"

// E-Ink Display Configuration
#define EPD_CS 5
#define EPD_DC 17
#define EPD_RST 16
#define EPD_BUSY 4

GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(GxEPD2_213_BN(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
QRCode qrcode;
uint8_t qrcodeData[128];  // Buffer for QR code data (adjust size based on version)


char g_ssid[32] = "ESP_AP";  // set AP SSID. Can be overwritten by creating a file on LittleFS with extension .ssid

// Do not touch the following variables
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;

const IPAddress localIP(4, 3, 2, 1);
const IPAddress gatewayIP(4, 3, 2, 1);
const IPAddress subnetMask(255, 255, 255, 0);
const char localIPURL[] = "http://4.3.2.1";

const int WIFI_CHANNEL = 1;

ConfigUtils config;


void setDisplayText(String header, String text) {
  display.setFullWindow();
  display.setTextColor(GxEPD_BLACK);
  display.firstPage();

  int x = 0;
  int y = 30;

  do {
    display.fillScreen(GxEPD_WHITE);

    // Print header
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(x, y);
    display.print(header);

    // Get height of header text
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(header, x, y, &tbx, &tby, &tbw, &tbh);

    // Move down by text height + padding
    y += tbh + 10;

    // Print body text
    display.setFont(&FreeSerif18pt7b);
    display.setCursor(x, y);
    display.print(text);
  } while (display.nextPage());
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
    String q = "";
    if (request->hasParam("question", true)) {
      q = request->getParam("question", true)->value();
    }
    setDisplayText(q, text);
    saveMessageToFile(text);
    sendData(text);
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


void drawQRCode(QRCode *qrcode) {
  int qrSize = qrcode->size;
  int pixelSize = 4;  // Adjust based on your display
  int xOffset = (display.width() - (qrSize * pixelSize)) / 2;
  int yOffset = (display.height() - (qrSize * pixelSize)) / 2;

  for (int y = 0; y < qrSize; y++) {
    for (int x = 0; x < qrSize; x++) {
      if (qrcode_getModule(qrcode, x, y)) {
        display.fillRect(
          xOffset + x * pixelSize,
          yOffset + y * pixelSize,
          pixelSize,
          pixelSize,
          GxEPD_BLACK);
      }
    }
  }
}

void showQRCode() {
  int bufferSize = qrcode_getBufferSize(3);
  uint8_t *qrcodeData = (uint8_t *)malloc(bufferSize);  // Dynamically allocate memory

  String wifiData = "WIFI:S:" + config.ssid + ";T:nopass;;";
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, wifiData.c_str());

  display.fillScreen(GxEPD_WHITE);
  drawQRCode(&qrcode);

  display.display();
}

void setup() {
  Serial.begin(115200);
  if (!initFileSystem()) {
    return;
  }
  listFiles();

  display.init(115200);
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);

  config.load();  // Load configuration
  strncpy(g_ssid, config.ssid.c_str(), sizeof(g_ssid) - 1);
  g_ssid[sizeof(g_ssid) - 1] = '\0';

  if (config.role == "ap") {
    startSoftAccessPoint(g_ssid, NULL, localIP, gatewayIP);
  } else {
    Serial.println("Starting WiFi in STA Mode");
    WiFi.mode(WIFI_STA);
  }
  Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
  Serial.printf("Wi-Fi channel: %d\n", WiFi.channel());

  setUpDNSServer(dnsServer, localIP);
  setUpWebserver(server, localIP);
  customEndPoints();

  InitESPNow();
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  initBroadcastClients();
  if (config.role == "AP") {
    showQRCode();
  }
}

void loop() {
  dnsServer.processNextRequest();
  ws.cleanupClients();
}