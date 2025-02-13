#include <LittleFS.h>
#include <DNSServer.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
// #include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <ArduinoJson.h>
#include <QRCodeGenerator.h>

// utils.
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

// Watchdog timer
const int wdtTimeout = 10000;  // tijd in ms tot watchdog ingrijpt
hw_timer_t *timer = NULL;

// Webserver
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
DNSServer dnsServer;

const IPAddress localIP(4, 3, 2, 1);
const IPAddress gatewayIP(4, 3, 2, 1);
const IPAddress subnetMask(255, 255, 255, 0);
const char localIPURL[] = "http://4.3.2.1";

// Configuration instance
ConfigUtils config;

int is_ap = 0;
String ssid = "unset";
int WIFI_CHANNEL = 1;
int topicIndex = 0;

// Timing / display
unsigned long nextWordTime = 0;
unsigned long minInterval = 1000;  // min interval in ms
unsigned long maxInterval = 2000;  // max interval in ms
String lastDisplayedWord = "";
bool newWordReceived = false;
String recievedWord = "";
String infoMode = "";
int infoInterval = 5;

/* Utility functions */

void enforceSSIDLimit() {
  /* 
  WiFi.softAP truncates the ssid to 31 + \0 chars. 
  Truncate here so we print the proper ssid
  */
  int byteCount = strlen(ssid.c_str());  // Get actual byte size

  // Trim while the byte count is over 31
  while (byteCount > 31) {
    ssid.remove(ssid.length() - 1);    // Remove last character
    byteCount = strlen(ssid.c_str());  // Recalculate size
  }
}

void ARDUINO_ISR_ATTR resetModule() {
  ets_printf("reboot\n");
  esp_restart();
}

String parseAndStoreRecievedWord(String recievedWord) {
  int topic = 0;
  String text = "";
  if (recievedWord.length() > 2 && recievedWord[1] == '|') {
    topic = recievedWord.substring(0, 1).toInt();
    text = recievedWord.substring(2);
  } else {
    topic = 0;
    text = recievedWord;
  }
  if (topic == topicIndex && !is_ap) {
    // store the text
    Serial.print("saving recieved text: ");
    Serial.println(text);
    saveMessageToFile(text);
  }
  return text;
}

/** End points **/

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
  if (LittleFS.exists("/messages.txt")) {
    request->send(LittleFS, "/messages.txt", "text/plain");
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
    nextWordTime = millis() + random(minInterval, maxInterval);  // make sure the word has time to display
    setDisplayText(q, text);
    saveMessageToFile(text);
    sendData(text);
    request->send(200, "text/plain", "Display updated with: " + text);
  } else {
    request->send(400, "text/plain", "No text provided!");
  }
}

void customEndPoints() {
  server.on("/message", HTTP_POST, handleMessageEndpoint);  // currently unused?
  server.on("/messages.txt", HTTP_GET, handleMessagesTxtEndpoint);
  server.on("/update-display", HTTP_POST, handleUpdateDisplayEndpoint);
}

/** Drawing methods **/


void setDisplayText(String header, String text) {
  display.setFullWindow();
  display.setTextColor(GxEPD_BLACK);
  // pick a random font
  display.setFont(&FreeSans12pt7b);
  int r = random(3);
  if (r == 1) {
    display.setFont(&FreeMonoBold12pt7b);
  } else if (r == 2) {
    display.setFont(&FreeSerifBold12pt7b);
  }

  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = 0;  // ((display.width() - tbw) / 2) - tbx;  // centering is problematic for second line
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(text);
  } while (display.nextPage());
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

  String wifiData = "WIFI:S:" + ssid + ";T:nopass;;";
  qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, wifiData.c_str());

  display.fillScreen(GxEPD_WHITE);
  drawQRCode(&qrcode);

  display.display();
}

// void helloWorld() {
//   display.setFullWindow();
//   display.setTextColor(GxEPD_BLACK);
//   int16_t tbx, tby;
//   uint16_t tbw, tbh;
//   display.getTextBounds("Hello World!", 0, 0, &tbx, &tby, &tbw, &tbh);
//   uint16_t x = ((display.width() - tbw) / 2) - tbx;
//   uint16_t y = ((display.height() - tbh) / 2) - tby;
//   display.firstPage();
//   do {
//     display.fillScreen(GxEPD_WHITE);
//     display.setCursor(x, y);
//     display.print("Hello World!");
//   } while (display.nextPage());
// }

void displayRandomMessage() {
  // std::vector<String> words = getMessagesFromFile();
  String word = getRandomMessage();
  Serial.println("Picked word: " + word);
  if (word != lastDisplayedWord) {
    setDisplayText("", word);
    lastDisplayedWord = word;
  }
  nextWordTime = millis() + random(minInterval, maxInterval);  // Schedule next display
}

/** Configuration **/

void loadConfig() {
  if (!config.load()) {
    Serial.println("ERROR: Config failed to load. Halting...");
    while (true)
      ;  // Stop execution if config is invalid
  }

  is_ap = config.getInt("is_ap");
  WIFI_CHANNEL = config.getInt("channel");
  topicIndex = config.getInt("topic");
  infoMode = config.getString("info_mode");
  infoInterval = config.getInt("info_interval");
  minInterval = config.getInt("min_interval");
  maxInterval = config.getInt("max_interval");

  JsonArray topics = config.getArray("topics");
  if (topics.size() == 0 || topicIndex < 0 || topicIndex >= (int)topics.size()) {
    Serial.println("ERROR: Invalid topic index or missing topics array.");
    while (true)
      ;  // Halt system
  }

  String topic = topics[topicIndex].as<String>();

  JsonArray ssids = config.getArray("ssids");
  if (ssids.size() == 0 || ssids.size() < topics.size()) {
    Serial.println("ERROR: Missing ssids array or size mismatch.");
    ssid = String(topic);
  } else {
    ssid = ssids[topicIndex].as<String>();
  }

  enforceSSIDLimit();

  Serial.println("Configuration Loaded:");
  Serial.print("is_ap: ");
  Serial.println(is_ap);
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Channel: ");
  Serial.println(WIFI_CHANNEL);
  Serial.print("Topic: ");
  Serial.println(topic);
}

void setup() {
  Serial.begin(115200);
  if (!initFileSystem()) {
    return;
  }
  listFiles();
  loadConfig();
  initializeMessagesFile();

  display.init(115200);
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);

  if (is_ap == 1) {
    startSoftAccessPoint(ssid.c_str(), NULL, localIP, gatewayIP);
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
  
  // Watchdog timeout op 10 seconden
  timer = timerBegin(1000000);                     // timer op 1MHz resolutie
  timerAttachInterrupt(timer, &resetModule);       // callback koppelen
  timerAlarm(timer, wdtTimeout * 1000, false, 0);  // timeout instellen in us

  pinMode(39, INPUT_PULLUP);
  if (digitalRead(39) == LOW) {  // hold btn during boot to see all stored messages
    printAllMessages();
  }

  nextWordTime = millis();  // Start right away
}

int wordCounter = 0;

void loop() {
  dnsServer.processNextRequest();
  ws.cleanupClients();

  if (newWordReceived) {
    newWordReceived = false;
    nextWordTime = millis() + random(minInterval, maxInterval);
    String word = parseAndStoreRecievedWord(recievedWord);
    setDisplayText("", word);
  } else if (millis() >= nextWordTime) {
    wordCounter++;
    if (infoMode == "QR" && wordCounter == infoInterval ) {
      wordCounter = 0;
      nextWordTime = millis() + random(minInterval, maxInterval);  // Schedule next display
      showQRCode();
    } else if ( infoMode == "SSID" && wordCounter == infoInterval) {
      wordCounter = 0;
      nextWordTime = millis() + random(minInterval, maxInterval);  // Schedule next display
      setDisplayText("", ssid);
    } else {
      displayRandomMessage();
    }
  }

  timerWrite(timer, 0);  // reset timer (feed watchdog)
  if (millis() > 86400000) {  // reboot the ESP32 every 24h.
    ESP.restart();
  }
}