#include "DebugUtils.h"
#include "WebServerUtils.h"

// Functies voor bestandshandeling
void listFiles() {
    DEBUG_VERBOSE("Listing files on LittleFS:");
    File root = LittleFS.open("/");
    if (!root) {
        DEBUG_ERROR("Failed to open root directory");
        return;
    }

    while (File file = root.openNextFile()) {
        DEBUG_VERBOSE("FILE: %s | SIZE: %d bytes\n", file.name(), file.size());
        file.close();
    }
}

void getSSIDFromFS() {
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
        String n = file.name();
        int dot = n.lastIndexOf(".");
        String ext = n.substring(dot);
        if (ext == ".ssid") {
            String hostName;
            if (n.substring(0, 1) == "/") {
                hostName = n.substring(1, dot);
            } else {
                hostName = n.substring(0, dot);
            }
            hostName.toCharArray(g_ssid, sizeof(g_ssid));
            break;
        }
        file = root.openNextFile();
    }
}

// Functies voor netwerk en serverinstellingen
void setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP) {
    dnsServer.setTTL(3600);
    dnsServer.start(53, "*", localIP);
}

void startSoftAccessPoint(const char *ssid, const char *password, const IPAddress &localIP, const IPAddress &gatewayIP) {
    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAPConfig(localIP, gatewayIP, subnetMask);
    WiFi.softAP(ssid, password);
}

void setUpWebserver(AsyncWebServer &server, const IPAddress &localIP) {
    // Basisroute voor statische bestanden
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // Routes specifiek voor netwerkconnectiviteit
    server.on("/connecttest.txt", [](AsyncWebServerRequest *request) {
        request->redirect("http://logout.net");
    });

    server.on("/wpad.dat", [](AsyncWebServerRequest *request) {
        request->send(404);
    });

    server.on("/generate_204", [](AsyncWebServerRequest *request) {
        request->redirect(localIPURL);
    });

    server.on("/redirect", [](AsyncWebServerRequest *request) {
        request->redirect(localIPURL);
    });

    server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request) {
        request->redirect(localIPURL);
    });

    server.on("/canonical.html", [](AsyncWebServerRequest *request) {
        request->redirect(localIPURL);
    });

    server.on("/success.txt", [](AsyncWebServerRequest *request) {
        request->send(200);
    });

    server.on("/ncsi.txt", [](AsyncWebServerRequest *request) {
        request->redirect(localIPURL);
    });

    // Favicon niet gevonden
    server.on("/favicon.ico", [](AsyncWebServerRequest *request) {
        request->send(404);
    });

    // Fallback voor niet-gevonden routes
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (LittleFS.exists(request->url())) {
            request->send(LittleFS, request->url(), String(), false);
            DEBUG_VERBOSE("Served " + request->url() + " from LittleFS");
        } else {
            request->redirect(localIPURL);
            DEBUG_VERBOSE("onNotFound: ");
            DEBUG_VERBOSE(request->url());
        }
    });

    // WebSocket handler
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    server.begin();
}

// Functies voor WebSocket
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->opcode == WS_TEXT) {
        data[len] = 0; // Zorg dat het een null-terminated string is
        if (strcmp((char *)data, "ping") == 0) {
            ws.textAll("pong");
        }
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        handleWebSocketMessage(arg, data, len);
    }
}