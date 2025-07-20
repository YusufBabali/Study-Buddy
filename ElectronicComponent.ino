/*
  ESP32 Web LED & Passive Buzzer Control Example + Countdown Timer + Format Button
  - LED on GPIO 2
  - Passive buzzer on GPIO 4
  - Format button on GPIO 15 (active LOW)
  - Web interface for countdown timer
*/

#include <WiFi.h>
#include <WebServer.h>
#include <SD.h>

const char* ssid = "Babalilar";
const char* password = "Laylaylom14.";

#define LED_PIN 2
#define BUZZER_PIN 4
#define FORMAT_BTN_PIN 15
#define SD_CS_PIN 5

WebServer server(80);

// Countdown state
volatile bool countdownActive = false;
volatile unsigned long countdownStartMillis = 0;
volatile int countdownDuration = 30;
volatile int blinkStart = 10;
volatile int buzzerStart = 5;
volatile int countdownRemaining = 0;
volatile bool restartedByButton = false;

// Melody for passive buzzer (simple beep-beep)
const int melody[] = { 262, 0, 262, 0, 262, 0 }; // C note, silence, repeat
const int noteDurations[] = { 150, 100, 150, 100, 150, 100 };

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
  <meta charset='UTF-8'>
  <title>ESP32 LED, Buzzer & Countdown</title>
  <style>
    body { font-family: Arial, sans-serif; background: #f0f0f0; text-align: center; margin-top: 40px; }
    h1 { color: #1976d2; }
    button { font-size: 1.2em; padding: 12px 32px; margin: 10px; border: none; border-radius: 8px; cursor: pointer; }
    .start { background: #1976d2; color: #fff; }
    .input-block { margin: 20px auto; display: inline-block; text-align: left; }
    label { font-weight: bold; }
    input[type=number] { font-size: 1.1em; width: 70px; margin-left: 8px; }
    #countdown { font-size: 2.5em; color: #d32f2f; margin: 20px 0; }
    #msg { color: #388e3c; font-size: 1.1em; min-height: 1.5em; }
  </style>
</head>
<body>
  <h1>ESP32 LED, Buzzer & Countdown</h1>
  <div class='input-block'>
    <div><label>Countdown Time (s):</label> <input type='number' id='cdTime' min='5' max='300' value='%d'></div>
    <div><label>Red LED Blink Start (s left):</label> <input type='number' id='blinkTime' min='1' max='60' value='%d'></div>
    <div><label>Buzzer Melody Start (s left):</label> <input type='number' id='buzzTime' min='1' max='60' value='%d'></div>
    <button class='start' id='startBtn'>Start Countdown</button>
  </div>
  <div id='countdown'>Ready</div>
  <div id='msg'></div>
  <script>
    document.getElementById('startBtn').onclick = async function() {
      const cd = document.getElementById('cdTime').value;
      const blink = document.getElementById('blinkTime').value;
      const buzz = document.getElementById('buzzTime').value;
      await fetch(`/start_countdown?cd=${cd}&blink=${blink}&buzz=${buzz}`);
    };
    async function pollCountdown() {
      const res = await fetch('/countdown_status');
      const data = await res.json();
      if (data.active) {
        document.getElementById('countdown').innerText = data.remaining + ' s';
      } else {
        document.getElementById('countdown').innerText = 'Ready';
      }
      if (data.restarted) {
        document.getElementById('msg').innerText = 'Restarted by button!';
        setTimeout(() => { document.getElementById('msg').innerText = ''; }, 2000);
      }
      setTimeout(pollCountdown, 500);
    }
    pollCountdown();
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  char page[sizeof(MAIN_page) + 100]; // Allocate enough space for sprintf
  sprintf(page, MAIN_page, countdownDuration, blinkStart, buzzerStart);
  server.send(200, "text/html", page);
}

// --- SD Card Config ---
void loadConfigFromSD() {
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[SD] SD Card Mount Failed! Using defaults.");
    countdownDuration = 30;
    blinkStart = 10;
    buzzerStart = 5;
    return;
  }
  File f = SD.open("/countdown.cfg", FILE_READ);
  if (!f) {
    Serial.println("[SD] Config not found. Using defaults.");
    countdownDuration = 30;
    blinkStart = 10;
    buzzerStart = 5;
    return;
  }
  String line = f.readStringUntil('\n');
  if (line.length() > 0) countdownDuration = line.toInt();
  line = f.readStringUntil('\n');
  if (line.length() > 0) blinkStart = line.toInt();
  line = f.readStringUntil('\n');
  if (line.length() > 0) buzzerStart = line.toInt();
  f.close();
  Serial.printf("[SD] Loaded config: countdown=%d, blink=%d, buzz=%d\n", countdownDuration, blinkStart, buzzerStart);
}

void saveConfigToSD() {
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[SD] SD Card Mount Failed! Cannot save config.");
    return;
  }
  File f = SD.open("/countdown.cfg", FILE_WRITE);
  if (!f) {
    Serial.println("[SD] Failed to open config for writing!");
    return;
  }
  f.printf("%d\n%d\n%d\n", countdownDuration, blinkStart, buzzerStart);
  f.close();
  Serial.println("[SD] Config saved.");
}

void handleStartCountdown() {
  if (server.hasArg("cd")) countdownDuration = server.arg("cd").toInt();
  if (server.hasArg("blink")) blinkStart = server.arg("blink").toInt();
  if (server.hasArg("buzz")) buzzerStart = server.arg("buzz").toInt();
  saveConfigToSD();
  countdownStartMillis = millis();
  countdownActive = true;
  countdownRemaining = countdownDuration;
  restartedByButton = false;
  server.send(200, "text/plain", "Countdown started");
}

void handleCountdownStatus() {
  String json = "{";
  json += "\"active\":"; json += (countdownActive ? "true" : "false");
  json += ",\"remaining\":"; json += countdownRemaining;
  json += ",\"restarted\":"; json += (restartedByButton ? "true" : "false");
  json += "}";
  restartedByButton = false;
  server.send(200, "application/json", json);
}

void doCountdownLogic() {
  static unsigned long lastUpdate = 0;
  static bool ledBlinking = false;
  static bool buzzerPlaying = false;
  static int melodyStep = 0;
  static unsigned long melodyStepStart = 0;

  // --- Format button logic (active LOW) ---
  static bool lastBtnState = HIGH;
  static unsigned long lastDebounce = 0;
  bool btnState = digitalRead(FORMAT_BTN_PIN);
  if (btnState != lastBtnState) {
    lastDebounce = millis();
    lastBtnState = btnState;
  }
  if (btnState == LOW && (millis() - lastDebounce) > 50) { // Button pressed
    if (countdownActive) {
      countdownStartMillis = millis();
      countdownRemaining = countdownDuration;
      restartedByButton = true;
    }
  }

  if (!countdownActive) {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
    ledBlinking = false;
    buzzerPlaying = false;
    return;
  }

  unsigned long elapsed = (millis() - countdownStartMillis) / 1000;
  int remaining = countdownDuration - elapsed;
  if (remaining < 0) remaining = 0;
  countdownRemaining = remaining;

  // LED blinking logic
  if (remaining <= blinkStart && remaining > 0) {
    if (!ledBlinking) {
      ledBlinking = true;
      digitalWrite(LED_PIN, HIGH);
      lastUpdate = millis();
    }
    if (millis() - lastUpdate > 300) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastUpdate = millis();
    }
  } else {
    if (ledBlinking) {
      digitalWrite(LED_PIN, LOW);
      ledBlinking = false;
    }
  }

  // Buzzer melody logic
  if (remaining <= buzzerStart && remaining > 0) {
    if (!buzzerPlaying) {
      buzzerPlaying = true;
      melodyStep = 0;
      melodyStepStart = millis();
    }
    if (buzzerPlaying && melodyStep < 6) {
      if (millis() - melodyStepStart > noteDurations[melodyStep]) {
        if (melody[melodyStep] > 0)
          tone(BUZZER_PIN, melody[melodyStep]);
        else
          noTone(BUZZER_PIN);
        melodyStepStart = millis();
        melodyStep++;
      }
    } else if (melodyStep >= 6) {
      noTone(BUZZER_PIN);
      melodyStep = 0;
      melodyStepStart = millis();
    }
  } else {
    if (buzzerPlaying) {
      noTone(BUZZER_PIN);
      buzzerPlaying = false;
    }
  }

  // End countdown
  if (remaining == 0 && countdownActive) {
    countdownActive = false;
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(FORMAT_BTN_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  delay(1000);
  loadConfigFromSD();
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  server.on("/", handleRoot);
  server.on("/start_countdown", handleStartCountdown);
  server.on("/countdown_status", handleCountdownStatus);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  doCountdownLogic();
} 