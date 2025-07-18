#include <SPI.h>
#include <SD.h>

// Change this to match your SD card module's CS pin
#define SD_CS_PIN 5



void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Serial test OK!");
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Card Mount Failed!");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) Serial.println("MMC");
  else if (cardType == CARD_SD) Serial.println("SDSC");
  else if (cardType == CARD_SDHC) Serial.println("SDHC");
  else Serial.println("UNKNOWN");

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  // Create and write to a file
  File file = SD.open("/test.txt", FILE_WRITE);
  if (file) {
    file.println("Hello from ESP32 SD card!");
    file.close();
    Serial.println("File written: /test.txt");
  } else {
    Serial.println("Failed to open file for writing");
  }

  // Read the file
  file = SD.open("/test.txt");
  if (file) {
    Serial.println("Reading file: /test.txt");
    while (file.available()) {
      Serial.write(file.read());
    }
    file.close();
    Serial.println();
  } else {
    Serial.println("Failed to open file for reading");
  }

  // Delete the file
  if (SD.remove("/test.txt")) {
    Serial.println("File deleted: /test.txt");
  } else {
    Serial.println("Failed to delete file: /test.txt");
  }
}

void loop() {
  // Nothing to do here
} 