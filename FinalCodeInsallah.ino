/*
  ESP32 WiFi + SD Card + Study Scheduler + RTC
  Connects to existing WiFi network, gets time from NTP, and stores data on SD card
  
  WiFi: Babalilar
  Password: Laylaylom14.
  SD Card CS Pin: 5
  RTC Module: DS3231 (I2C)
*/

#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <time.h>
#include <RTClib.h>
#include <ArduinoJson.h>

// --- Wi-Fi and SD Card Configuration ---
const char* ssid = "Babalilar";
const char* password = "Laylaylom14.";
#define SD_CS_PIN 5

// --- RTC and NTP Configuration ---
RTC_DS3231 rtc;
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0; // Adjust for your timezone
const int daylightOffset_sec = 3600;

WebServer server(80);

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Study Session Scheduler</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <style>
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            margin: 0;
            background: linear-gradient(120deg, #fff700 0%, #ffe082 100%);
            min-height: 100vh;
        }
        header {
            background: #1976d2;
            color: #fff;
            padding: 20px 0 10px 0;
            text-align: center;
            box-shadow: 0 2px 8px #b0c4de44;
        }
        h1 {
            margin: 0;
            font-size: 2.2em;
            letter-spacing: 1px;
        }
        .container {
            max-width: 700px;
            margin: 30px auto 0 auto;
            padding: 0 10px 30px 10px;
        }
        .day-block, .config-section {
            background: #fff;
            border-radius: 12px;
            box-shadow: 0 4px 16px #b0c4de44;
            margin-bottom: 24px;
            padding: 18px 18px 12px 18px;
            transition: box-shadow 0.2s;
        }
        .day-block:hover, .config-section:hover {
            box-shadow: 0 8px 24px #1976d233;
        }
        .day-block h2 {
            margin-top: 0;
            color: #1976d2;
        }
        .lectures { margin-bottom: 10px; }
        .lecture-row {
            display: flex;
            gap: 10px;
            margin-bottom: 8px;
            align-items: center;
        }
        .lecture-row input[type="time"] {
            width: 100px;
            padding: 3px 6px;
            border-radius: 4px;
            border: 1px solid #b0c4de;
        }
        .add-lecture {
            margin-bottom: 10px;
            background: #43a047;
            color: #fff;
            border: none;
            padding: 6px 14px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 1em;
            transition: background 0.2s;
        }
        .add-lecture i { margin-right: 4px; }
        .add-lecture:hover { background: #388e3c; }
        .remove-btn {
            background: #e53935;
            color: #fff;
            border: none;
            border-radius: 4px;
            padding: 5px 10px;
            cursor: pointer;
            font-size: 1em;
            transition: background 0.2s;
        }
        .remove-btn i { margin-right: 2px; }
        .remove-btn:hover { background: #b71c1c; }
        .config-section h2 {
            color: #1976d2;
            margin-top: 0;
        }
        .config-section label {
            display: block;
            margin-bottom: 10px;
        }
        .save-btn {
            width: 100%;
            font-size: 1.1em;
            margin-top: 20px;
            background: #1976d2;
            color: #fff;
            border: none;
            border-radius: 6px;
            padding: 10px 0;
            cursor: pointer;
            transition: background 0.2s;
        }
        .save-btn:hover { background: #1565c0; }
        /* Statistics Table */
        .stats-section {
            background: #fff;
            border-radius: 12px;
            box-shadow: 0 4px 16px #b0c4de44;
            margin-bottom: 24px;
            padding: 18px 18px 12px 18px;
            margin-top: 0;
        }
        .stats-section h2 {
            color: #1976d2;
            margin-top: 0;
        }
        .stats-table {
            width: 100%;
            border-collapse: collapse;
            background: #f9f9f9;
            border-radius: 8px;
            overflow: hidden;
        }
        .stats-table th, .stats-table td {
            padding: 8px;
            border-bottom: 1px solid #b0c4de;
            text-align: center;
        }
        .stats-table th {
            background: #e3eafc;
        }
        .stats-table tr:last-child td {
            border-bottom: none;
        }
        footer {
            text-align: center;
            color: #888;
            font-size: 0.95em;
            padding: 18px 0 10px 0;
        }
    </style>
</head>
<body>
    <header>
        <h1>Study Session Scheduler</h1>
    </header>
    <div class="container">
        <div style="margin-bottom: 20px;">
            <label for="monthSelect" style="font-weight: bold; font-size: 1.1em;">Select Month: </label>
            <select id="monthSelect" name="monthSelect" style="font-size: 1.1em; padding: 4px 10px; border-radius: 6px;">
                <option value="1">January</option>
                <option value="2">February</option>
                <option value="3">March</option>
                <option value="4">April</option>
                <option value="5">May</option>
                <option value="6">June</option>
                <option value="7">July</option>
                <option value="8">August</option>
                <option value="9">September</option>
                <option value="10">October</option>
                <option value="11">November</option>
                <option value="12">December</option>
            </select>
            <label for="weekSelect" style="font-weight: bold; font-size: 1.1em; margin-left: 20px;">Select Week: </label>
            <select id="weekSelect" name="weekSelect" style="font-size: 1.1em; padding: 4px 10px; border-radius: 6px;">
                <option value="1">Week 1</option>
                <option value="2">Week 2</option>
                <option value="3">Week 3</option>
                <option value="4">Week 4</option>
            </select>
            <span id="weekStatus" style="margin-left: 15px; color: #e53935; font-weight: bold;"></span>
        </div>
        <form id="scheduleForm">
            <div id="daysContainer"></div>
            <div class="config-section">
                <h2>Configuration</h2>
                <label>Response Window (seconds): <input type="number" id="responseWindow" min="10" max="300" value="30"></label>
                <label>Red LED Warning (minutes before end): <input type="number" id="redLedWarning" min="1" max="30" value="5"></label>
                <label>Red LED Blink (seconds before end): <input type="number" id="redLedBlink" min="5" max="120" value="30"></label>
            </div>
            <button type="submit" class="save-btn" id="saveBtn"><i class="fa-solid fa-floppy-disk"></i> Save Schedule</button>
        </form>
    </div>
    <!-- Statistics Section -->
    <div class="container">
        <div class="stats-section">
            <h2>Session Statistics (This Week)</h2>
            <table class="stats-table">
                <thead>
                    <tr>
                        <th>Day</th>
                        <th>Session Time</th>
                        <th>Success</th>
                        <th>Missed Button Presses</th>
                    </tr>
                </thead>
                <tbody id="statsTableBody">
                    <!-- Filled by JS -->
                </tbody>
            </table>
        </div>
    </div>
    <footer>
        &copy; 2024 Study Session Scheduler | ESP32 Project
    </footer>
    <script>
document.addEventListener('DOMContentLoaded', function() {
    const days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
    const daysContainer = document.getElementById('daysContainer');
    const weekSelect = document.getElementById('weekSelect');
    const monthSelect = document.getElementById('monthSelect');
    const weekStatus = document.getElementById('weekStatus');
    const saveBtn = document.getElementById('saveBtn');

    // Render days for one week only
    daysContainer.innerHTML = '';
    days.forEach(day => {
        const block = document.createElement('div');
        block.className = 'day-block';
        block.innerHTML = `<h2>${day}</h2>
            <div class="lectures" id="lectures-${day}"></div>
            <button type="button" class="add-lecture"><i class="fa-solid fa-plus"></i> Add Lecture</button>`;
        daysContainer.appendChild(block);
        block.querySelector('.add-lecture').addEventListener('click', function() {
            addLecture(day);
        });
    });

    function addLecture(day) {
        const lecturesDiv = document.getElementById(`lectures-${day}`);
        const row = document.createElement('div');
        row.className = 'lecture-row';
        row.innerHTML = `
            <label>Start: <input type="time" name="${day}-start[]" required></label>
            <label>End: <input type="time" name="${day}-end[]" required></label>
            <button type="button" class="remove-btn"><i class='fa-solid fa-trash'></i> Remove</button>
        `;
        row.querySelector('.remove-btn').addEventListener('click', function() {
            row.remove();
        });
        lecturesDiv.appendChild(row);
    }

    // Week existence check
    function checkWeekExists(month, week) {
        weekStatus.textContent = '';
        saveBtn.disabled = true;
        fetch(`/check_week?month=${month}&week=${week}`)
            .then(res => res.json())
            .then(data => {
                if (data.exists) {
                    weekStatus.textContent = 'This week\'s program is already prepared!';
                    saveBtn.disabled = true;
                } else {
                    weekStatus.textContent = '';
                    saveBtn.disabled = false;
                }
            })
            .catch(() => {
                weekStatus.textContent = 'Could not check week status.';
                saveBtn.disabled = true;
            });
    }

    // Initial check
    checkWeekExists(monthSelect.value, weekSelect.value);
    weekSelect.addEventListener('change', function() {
        checkWeekExists(monthSelect.value, this.value);
    });
    monthSelect.addEventListener('change', function() {
        checkWeekExists(this.value, weekSelect.value);
    });

    document.getElementById('scheduleForm').onsubmit = function(e) {
        e.preventDefault();
        if (saveBtn.disabled) return;
        // Collect form data
        const formData = new FormData();
        formData.append('month', monthSelect.value);
        formData.append('week', weekSelect.value);
        formData.append('responseWindow', document.getElementById('responseWindow').value);
        formData.append('redLedWarning', document.getElementById('redLedWarning').value);
        formData.append('redLedBlink', document.getElementById('redLedBlink').value);
        // Collect schedule data
        days.forEach(day => {
            const starts = document.querySelectorAll(`input[name="${day}-start[]"]`);
            const ends = document.querySelectorAll(`input[name="${day}-end[]"]`);
            starts.forEach((start, index) => {
                if (start.value && ends[index] && ends[index].value) {
                    formData.append(`${day}_start_${index}`, start.value);
                    formData.append(`${day}_end_${index}`, ends[index].value);
                }
            });
        });
        fetch('/save', {
            method: 'POST',
            body: formData
        })
        .then(response => response.text())
        .then(data => {
            alert('Schedule saved successfully!');
            checkWeekExists(monthSelect.value, weekSelect.value);
        })
        .catch(error => {
            alert('Error saving schedule: ' + error);
        });
    };

    // --- Statistics Table JS ---
    // Placeholder statistics data
    const statsData = [
        { day: "Monday", time: "15:00–15:45", success: true, missed: 0 },
        { day: "Monday", time: "16:00–16:45", success: false, missed: 1 },
        { day: "Tuesday", time: "14:00–14:45", success: true, missed: 0 },
        { day: "Wednesday", time: "13:00–13:45", success: false, missed: 2 },
        { day: "Thursday", time: "15:00–15:45", success: true, missed: 0 },
        { day: "Friday", time: "16:00–16:45", success: true, missed: 0 },
        { day: "Saturday", time: "14:00–14:45", success: true, missed: 0 },
        { day: "Sunday", time: "13:00–13:45", success: true, missed: 0 }
    ];
    function renderStatsTable() {
        const tbody = document.getElementById('statsTableBody');
        tbody.innerHTML = '';
        statsData.forEach(row => {
            tbody.innerHTML += `
                <tr>
                    <td>${row.day}</td>
                    <td>${row.time}</td>
                    <td style="color:${row.success ? '#43a047' : '#e53935'};">
                        ${row.success ? '<i class="fa-solid fa-check-circle"></i> Yes' : '<i class="fa-solid fa-xmark-circle"></i> No'}
                    </td>
                    <td>${row.missed}</td>
                </tr>
            `;
        });
    }
    renderStatsTable();
});
</script>
</body>
</html>
)rawliteral";

// --- Helper: Get month name from number ---
String getMonthName(int month) {
  const char* months[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
  if (month < 1 || month > 12) return "Unknown";
  return String(months[month - 1]);
}

// --- Endpoint: Check if a week schedule exists ---
void handleCheckWeek() {
  int month = server.hasArg("month") ? server.arg("month").toInt() : 0;
  int week = server.hasArg("week") ? server.arg("week").toInt() : 0;
  String monthName = getMonthName(month);
  String folder = "/" + monthName + "_Week" + String(week);
  String filename = folder + "/schedule.txt";
  bool exists = SD.exists(filename);
  StaticJsonDocument<64> doc;
  doc["exists"] = exists;
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// --- Endpoint: Save schedule for selected week ---
void handleSave() {
  String response = "";
  DateTime now = rtc.now();
  String currentTime = String(now.year()) + "-" + 
                      String(now.month() < 10 ? "0" : "") + String(now.month()) + "-" + 
                      String(now.day() < 10 ? "0" : "") + String(now.day()) + " " +
                      String(now.hour() < 10 ? "0" : "") + String(now.hour()) + ":" +
                      String(now.minute() < 10 ? "0" : "") + String(now.minute()) + ":" +
                      String(now.second() < 10 ? "0" : "") + String(now.second());

  // Get month and week from POST data
  int month = server.hasArg("month") ? server.arg("month").toInt() : 0;
  int week = server.hasArg("week") ? server.arg("week").toInt() : 0;
  String monthName = getMonthName(month);
  String folder = "/" + monthName + "_Week" + String(week);
  String filename = folder + "/schedule.txt";

  // Prevent overwrite
  if (SD.exists(filename)) {
    response = "Error: Schedule for this week already exists!";
    server.send(409, "text/plain", response);
    return;
  }

  // Create folder if not exists
  if (!SD.exists(folder)) {
    SD.mkdir(folder.c_str());
  }

  File file = SD.open(filename, FILE_WRITE);
  if (file) {
    file.println("STUDY SESSION SCHEDULE WITH RTC");
    file.println("================================");
    file.println("Generated: " + currentTime);
    file.println();
    file.println("CONFIGURATION");
    file.println("-------------");
    if (server.hasArg("responseWindow")) file.print("Response Window: "); file.println(server.arg("responseWindow") + " seconds");
    if (server.hasArg("redLedWarning")) file.print("Red LED Warning: "); file.println(server.arg("redLedWarning") + " minutes");
    if (server.hasArg("redLedBlink")) file.print("Red LED Blink: "); file.println(server.arg("redLedBlink") + " seconds");
    if (server.hasArg("sessionDuration")) file.print("Session Duration: "); file.println(server.arg("sessionDuration") + " minutes");
    file.println();
    file.println("WEEKLY STUDY SCHEDULE");
    file.println("=====================");
    file.println();
    file.println("WEEKDAYS (Monday - Friday):");
    file.println("---------------------------");
    const char* weekdays[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
    for (int d = 0; d < 5; d++) {
      file.println();
      file.println(weekdays[d] + String(":"));
      file.println("--------");
      int index = 0;
      while (server.hasArg(String(weekdays[d]) + "_start_" + String(index))) {
        String start = server.arg(String(weekdays[d]) + "_start_" + String(index));
        String end = server.arg(String(weekdays[d]) + "_end_" + String(index));
        String subject = server.hasArg(String(weekdays[d]) + "_subject_" + String(index)) ? server.arg(String(weekdays[d]) + "_subject_" + String(index)) : "";
        file.print("  Session "); file.print(index + 1); file.println(":");
        file.print("    Time: "); file.print(start); file.print(" - "); file.println(end);
        if (subject.length() > 0) {
          file.print("    Subject: "); file.println(subject);
        }
        file.println();
        index++;
      }
      if (index == 0) {
        file.println("  No sessions scheduled");
      }
    }
    file.println();
    file.println("WEEKEND (Saturday - Sunday):");
    file.println("----------------------------");
    const char* weekends[] = {"Saturday", "Sunday"};
    for (int d = 0; d < 2; d++) {
      file.println();
      file.println(weekends[d] + String(":"));
      file.println("--------");
      int index = 0;
      while (server.hasArg(String(weekends[d]) + "_start_" + String(index))) {
        String start = server.arg(String(weekends[d]) + "_start_" + String(index));
        String end = server.arg(String(weekends[d]) + "_end_" + String(index));
        String subject = server.hasArg(String(weekends[d]) + "_subject_" + String(index)) ? server.arg(String(weekends[d]) + "_subject_" + String(index)) : "";
        file.print("  Session "); file.print(index + 1); file.println(":");
        file.print("    Time: "); file.print(start); file.print(" - "); file.println(end);
        if (subject.length() > 0) {
          file.print("    Subject: "); file.println(subject);
        }
        file.println();
        index++;
      }
      if (index == 0) {
        file.println("  No sessions scheduled");
      }
    }
    file.println("=====================");
    file.println("End of Schedule");
    file.println("RTC Time: " + currentTime);
    file.close();
    response += "File saved: " + filename + "\n";
    response += "RTC Time: " + currentTime;
    Serial.println("Schedule saved to SD card: " + filename);
    Serial.println("RTC Time: " + currentTime);
    server.send(200, "text/plain", response);
  } else {
    response = "Error: Could not save to SD card!";
    Serial.println("Failed to save schedule to SD card");
    server.send(500, "text/plain", response);
  }
}

// --- TODO: Add /get_schedule endpoint for two-way sync in the future ---

void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize

  Serial.println("ESP32 WiFi + SD Card + Study Scheduler + RTC");
  Serial.println("============================================");

  // Initialize I2C for RTC
  Wire.begin();

  // Initialize RTC
  Serial.println("Initializing RTC module...");
  if (!rtc.begin()) {
    Serial.println("❌ RTC module not found!");
    Serial.println("Please check your RTC module connection.");
    return;
  }
  if (rtc.lostPower()) {
    Serial.println("⚠️ RTC lost power, setting time from NTP...");
  }

  // Initialize SD card
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("❌ SD Card Mount Failed!");
    Serial.println("Please check your SD card connection and try again.");
    return;
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("❌ No SD card attached");
    return;
  }
  Serial.print("✅ SD Card Type: ");
  if (cardType == CARD_MMC) Serial.println("MMC");
  else if (cardType == CARD_SD) Serial.println("SDSC");
  else if (cardType == CARD_SDHC) Serial.println("SDHC");
  else Serial.println("UNKNOWN");
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("📊 SD Card Size: %lluMB\n", cardSize);

  // Connect to WiFi network
  Serial.println();
  Serial.println("Connecting to WiFi network...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi connected successfully!");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.println("Configuring NTP...");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Waiting for NTP time sync...");
    time_t now = 0;
    int ntpAttempts = 0;
    while (now < 24 * 3600 && ntpAttempts < 10) {
      delay(1000);
      now = time(nullptr);
      ntpAttempts++;
      Serial.print(".");
    }
    if (now > 24 * 3600) {
      Serial.println();
      Serial.println("✅ NTP time sync successful!");
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
        Serial.println("✅ RTC time set from NTP");
      }
    } else {
      Serial.println();
      Serial.println("⚠️ NTP time sync failed, using RTC time");
    }
  } else {
    Serial.println();
    Serial.println("❌ Failed to connect to WiFi!");
    Serial.println("Please check your WiFi credentials and try again.");
    return;
  }
  DateTime now = rtc.now();
  Serial.println();
  Serial.println("🕐 Current RTC Time:");
  Serial.printf("Date: %04d-%02d-%02d\n", now.year(), now.month(), now.day());
  Serial.printf("Time: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
  Serial.printf("Day of Week: %d\n", now.dayOfTheWeek());

  // --- Web server routes ---
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/check_week", HTTP_GET, handleCheckWeek);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("✅ HTTP server started on port 80");
  Serial.print("🌐 Access the web interface at: http://");
  Serial.println(WiFi.localIP());
  Serial.println("============================================");
}

void loop() {
  server.handleClient();
  delay(10); // Small delay to prevent watchdog issues
} 