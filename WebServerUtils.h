#ifndef SERVERUTILS_H
#define SERVERUTILS_H

#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <DNSServer.h>
#include <IPAddress.h>

// Externe variabelen
extern char g_ssid[32];
extern AsyncWebServer server; 
extern AsyncWebSocket ws;
extern const IPAddress localIP;
extern const IPAddress gatewayIP;
extern const IPAddress subnetMask;
extern const char localIPURL[];
extern int WIFI_CHANNEL;
extern int topicIndex;

// Functies voor bestandshandeling
void listFiles();
void getSSIDFromFS();

// Functies voor netwerk en serverinstellingen
void setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP);
void startSoftAccessPoint(const char *ssid, const char *password, const IPAddress &localIP, const IPAddress &gatewayIP);
void setUpWebserver(AsyncWebServer &server, const IPAddress &localIP);

// Functies voor WebSocket
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

#endif