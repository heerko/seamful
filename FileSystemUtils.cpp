#include "FileSystemUtils.h"

#define MAX_FILE_SIZE 800000  // 800KB limiet
#define MIN_TRIM_LINES 50     // Aantal regels om per keer te verwijderen

bool isTrimming = false;

bool initFileSystem() {
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return false;
  }
  return true;
}

void initializeMessagesFile() {
    Serial.println("Checking for first_run_flag.txt...");

    if (LittleFS.exists("/first_run_flag.txt")) {
        Serial.println("First run flag detected. Skipping initialization.");
        return; // Skip processing if already run
    }

    Serial.println("First run detected. Initializing messages from seed.txt...");

    File seedFile = LittleFS.open("/seed.txt", "r");
    if (!seedFile) {
        Serial.println("ERROR: Failed to open seed.txt");
        return;
    }
    Serial.println("seed.txt opened successfully.");

    File messageFile = LittleFS.open("/messages.txt", "w"); // Overwrite
    if (!messageFile) {
        Serial.println("ERROR: Failed to create messages.txt");
        seedFile.close();
        return;
    }
    Serial.println("messages.txt created successfully.");

    int linesRead = 0, linesWritten = 0;

    while (seedFile.available()) {
        String line = seedFile.readStringUntil('\n');
        linesRead++;

        Serial.print("Read line: ");
        Serial.println(line);

        if (line.startsWith(String(topicIndex) + "|")) {
            String message = line.substring(2); // Skip "0|" prefix
            messageFile.println(message);
            linesWritten++;

            Serial.print("Saved message: ");
            Serial.println(message);
        }
    }

    seedFile.close();
    messageFile.close();

    Serial.print("Finished processing seed.txt. Lines read: ");
    Serial.print(linesRead);
    Serial.print(", Messages written: ");
    Serial.println(linesWritten);

    // Create flag file to mark initialization as complete
    Serial.println("Creating first_run_flag.txt...");
    File flagFile = LittleFS.open("/first_run_flag.txt", "w");
    if (flagFile) {
        flagFile.println("Initialized");
        flagFile.close();
        Serial.println("First run flag created successfully.");
    } else {
        Serial.println("ERROR: Failed to create first_run_flag.txt");
    }
}

void saveMessageToFile(const String &message) {
  if (isTrimming) {
    Serial.println("Skipping save, trimming in progress...");
    return;  // Voorkomt schrijven tijdens trimming
  }

  if (!LittleFS.exists("/messages.txt")) {
    File file = LittleFS.open("/messages.txt", "w");
    if (!file) {
      Serial.println("Failed to create messages.txt for writing");
      return;
    }
    file.close();
  }

  // **Check bestandsgrootte vóór schrijven**
  File file = LittleFS.open("/messages.txt", "r");
  if (!file) {
    Serial.println("Failed to open messages.txt for size check");
    return;
  }

  size_t fileSize = file.size();
  file.close();

  if (fileSize >= MAX_FILE_SIZE) {
    Serial.println("File size exceeds limit, trimming...");
    trimOldMessages();
  }

  if (!isTrimming) {  // Dubbele check om race condition te vermijden
    file = LittleFS.open("/messages.txt", "a");
    if (file) {
      file.println(message);
      file.close();
      Serial.println("Message saved.");
    } else {
      Serial.println("Failed to open messages.txt for appending");
    }
  }
}


void trimTask(void *parameter) {
  isTrimming = true;  // Lock: trimming is bezig
  File file = LittleFS.open("/messages.txt", "r");
  if (!file) {
    Serial.println("Failed to open messages.txt for trimming");
    isTrimming = false;
    vTaskDelete(NULL);  // Beëindig de taak
    return;
  }

  size_t fileSize = file.size();
  if (fileSize <= MAX_FILE_SIZE) {
    file.close();
    isTrimming = false;
    vTaskDelete(NULL);
    return;  // Geen actie nodig
  }

  Serial.println("Trimming old messages...");

  // Dynamisch berekenen hoeveel regels we moeten verwijderen
  int linesToRemove = MIN_TRIM_LINES + ((fileSize - MAX_FILE_SIZE) / 5000);

  File tempFile = LittleFS.open("/messages_tmp.txt", "w");
  if (!tempFile) {
    Serial.println("Failed to create temp file");
    file.close();
    isTrimming = false;
    vTaskDelete(NULL);
    return;
  }

  int lineCount = 0;
  String buffer = "";
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (lineCount >= linesToRemove) {
      buffer += line + "\n";
      if (buffer.length() > 512) {  // Schrijf in blokken van 512 bytes
        tempFile.print(buffer);
        buffer = "";
        vTaskDelay(1);  // **Voorkomt Watchdog Timeout**
      }
    }
    lineCount++;

    if (lineCount % 100 == 0) {
      vTaskDelay(1);  // **Voorkomt Watchdog Timeout om de 100 regels**
    }
  }

  if (buffer.length() > 0) {
    tempFile.print(buffer);
  }

  file.close();
  tempFile.close();

  // **Forceer alle open bestandsobjecten te sluiten vóór we verwijderen**
  LittleFS.end();    // Unmount LittleFS (sluit alle bestanden)
  vTaskDelay(10);    // Even wachten om zeker te zijn
  LittleFS.begin();  // Mount opnieuw

  // Oude bestand vervangen
  if (!LittleFS.remove("/messages.txt")) {
    Serial.println("ERROR: Failed to remove old messages.txt");
  } else {
    Serial.println("Old messages.txt removed successfully.");
  }

  if (!LittleFS.rename("/messages_tmp.txt", "/messages.txt")) {
    Serial.println("ERROR: Failed to rename temp file.");
  } else {
    Serial.println("Old messages trimmed successfully.");
  }

  isTrimming = false;  // Unlock: trimming is klaar
  vTaskDelete(NULL);   // Beëindig de FreeRTOS-taak
}

void trimOldMessages() {
  if (isTrimming) return;  // Voorkomt dubbele trimming

  // **Start trimming in een aparte taak op Core 1**
  xTaskCreatePinnedToCore(
    trimTask,    // Functie
    "TrimTask",  // Naam
    4096,        // Stack grootte (4 KB)
    NULL,        // Parameter (niet nodig)
    1,           // Prioriteit (laag)
    NULL,        // Taak handle
    1            // Core 1 (ESP32 heeft 2 cores)
  );
}

/* 
Leaving it here, but this is probably probematic when 
the file size grows. 
*/
std::vector<String> getMessagesFromFile() {
  File file = LittleFS.open("/messages.txt", "r");
  if (!file) {
    Serial.println("Failed to open messages.txt");
    return {};
  }

  std::vector<String> words;
  while (file.available()) {
    words.push_back(file.readStringUntil('\n'));
  }
  file.close();

  return words;
}

String getRandomMessage() {
  File file = LittleFS.open("/messages.txt", "r");
  if (!file) {
    Serial.println("Failed to open file");
    return "";
  }

  int numLines = 0;
  while (file.available()) {
    file.readStringUntil('\n');
    numLines++;
  }
  file.close();

  if (numLines == 0) return "";

  int randomLine = random(0, numLines);
  file = LittleFS.open("/messages.txt", "r");

  int currentLine = 0;
  String message;
  while (file.available()) {
    message = file.readStringUntil('\n');
    if (currentLine == randomLine) break;
    currentLine++;
  }

  file.close();
  return message;
}

void printAllMessages() {
  File file = LittleFS.open("/messages.txt", "r");
  if (!file) {
    Serial.println("Failed to open messages.txt");
    return;
  }

  while (file.available()) {
    Serial.println(file.readStringUntil('\n'));
  }

  file.close();
}

void listFiles() {
  Serial.println("Listing files in LittleFS:");
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("File: %s (%d bytes)\n", file.name(), file.size());
    file = root.openNextFile();
  }
  Serial.print(LittleFS.usedBytes());
  Serial.print(" bytes used of ");
  Serial.print(LittleFS.totalBytes());
  Serial.println(" bytes total.");
}