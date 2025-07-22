#include "WebServerUtils.h"

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
  root.close();
}

// Functies voor netwerk en serverinstellingen
void setUpDNSServer(DNSServer &dnsServer, const IPAddress &localIP) {
  dnsServer.setTTL(3600);
  dnsServer.start(53, "*", localIP);
}

void startSoftAccessPoint(const char *ssid, const char *password, const IPAddress &localIP, const IPAddress &gatewayIP) {
  // WiFi.mode(WIFI_MODE_AP);
  WiFi.mode(WIFI_AP_STA);  // Enable both AP and STA modes for ESP-NOW
  WiFi.softAPConfig(localIP, gatewayIP, subnetMask);
  if (WiFi.softAP(ssid, password, WIFI_CHANNEL)) {
    Serial.println("Access Point started successfully");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start Access Point");
  }
}

// ---- Bestandenlijst ophalen ----
void handleListFiles(AsyncWebServerRequest *request) {
  String fileList = "[";
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    if (fileList.length() > 1) fileList += ",";
    fileList += "\"" + String(file.name()) + "\"";
    file = root.openNextFile();
  }
  fileList += "]";
  request->send(200, "application/json", fileList);
}

// ---- Bestand lezen ----
void handleEditFile(AsyncWebServerRequest *request) {
  if (!request->hasParam("file")) {
    request->send(400, "text/plain", "Missing file parameter");
    return;
  }

  String filename = request->getParam("file")->value();
  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }

  if (!LittleFS.exists(filename)) {
    Serial.print("File NOT found: ");
    Serial.println(filename);
    request->send(404, "text/plain", "File not found");
    return;
  }

  Serial.print("Streaming file: ");
  Serial.println(filename);

  request->send(LittleFS, filename, "text/plain", false);  // **Stream bestand**
}

// ---- Bestand opslaan via POST ----
void handleSaveFile(AsyncWebServerRequest *request) {
  if (!request->hasParam("file", true) || !request->hasParam("content", true)) {
    request->send(400, "text/plain", "Missing file or content parameter");
    return;
  }

  String filename = request->getParam("file", true)->value();
  String content = request->getParam("content", true)->value();

  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }

  Serial.print("Saving file: ");
  Serial.println(filename);

  File file = LittleFS.open(filename, "w");
  if (!file) {
    Serial.println("ERROR: Failed to open file for writing");
    request->send(500, "text/plain", "Failed to open file");
    return;
  }

  file.print(content);
  file.close();

  Serial.println("File saved successfully!");
  request->send(200, "text/plain", "File saved successfully");
}

void handleDownloadFile(AsyncWebServerRequest *request) {
  if (!request->hasParam("file")) {
    request->send(400, "text/plain", "Missing file parameter");
    return;
  }

  String filename = request->getParam("file")->value();

  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }

  if (!LittleFS.exists(filename)) {
    Serial.print("Download failed. File not found: ");
    Serial.println(filename);
    request->send(404, "text/plain", "File not found");
    return;
  }

  Serial.print("Serving file for download: ");
  Serial.println(filename);
  request->send(LittleFS, filename, String(), true);  // `true` forceert download
}

void handleDeleteFile(AsyncWebServerRequest *request) {
  if (request->args() == 0) {
    request->send(400, "text/plain", "Missing file parameter");
    return;
  }

  String filename = request->arg("file");

  if (!filename.startsWith("/")) {
    filename = "/" + filename;
  }

  if (!LittleFS.exists(filename)) {
    Serial.print("File NOT found: ");
    Serial.println(filename);
    request->send(404, "text/plain", "File not found");
    return;
  }

  if (LittleFS.remove(filename)) {
    Serial.print("Deleted file: ");
    Serial.println(filename);
    request->send(200, "text/plain", "File deleted successfully");
  } else {
    Serial.print("Failed to delete file: ");
    Serial.println(filename);
    request->send(500, "text/plain", "Failed to delete file");
  }
}

void handleReboot(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", "ESP32 will now reboot. Reload try a reload in 15s");
  delay(500);
  ESP.restart();
}


void setUpWebserver(AsyncWebServer &server, const IPAddress &localIP) {
  // Serve static files and ensure "/" loads index.html for captive portal
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // Override handling for /index.html to add topic parameter if missing
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Request for /index.html");
    if (!request->hasParam("topic")) {
      Serial.print("Redirecting to /index.html?topic=");
      Serial.println(topicIndex);
      request->redirect("/index.html?topic=" + String(topicIndex));
    } else {
      Serial.println("Serving index.html with existing topic param");
      request->send(LittleFS, "/index.html", "text/html");
    }
  });

  // Handle the root "/" explicitly to ensure captive portal opens
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Request for / -> Redirecting to /index.html");
    request->redirect("/index.html");
  });

  // Webpagina (editor)
  server.on("/editor", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/editor.html", "text/html");
  });

  // API routes
  server.on("/list", HTTP_GET, handleListFiles);
  server.on("/edit", HTTP_GET, handleEditFile);
  server.on("/save", HTTP_POST, handleSaveFile);
  server.on("/download", HTTP_GET, handleDownloadFile);
  server.on("/delete", HTTP_POST, handleDeleteFile);
  server.on("/reboot", HTTP_POST, handleReboot);

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
      AsyncWebServerResponse *response = request->beginResponse(LittleFS, request->url(), String(), false);
      response->addHeader("X-Topic-Index", String(topicIndex));  // Attach topicIdx to all file responses
      request->send(response);
      // request->send(LittleFS, request->url(), String(), false);
      Serial.print("Served from LittleFS: ");
      Serial.println(request->url());
    } else {
      request->redirect(localIPURL);
      Serial.print("onNotFound: ");
      Serial.println(request->url());
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
    data[len] = 0;  // Zorg dat het een null-terminated string is
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