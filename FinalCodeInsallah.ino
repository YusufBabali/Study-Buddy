/*
  Simple ESP32 WiFi Test
  Just connects to WiFi and shows if it worked or not
  
  WiFi: Babalilar
  Password: Laylaylom14.
*/

#include <WiFi.h>
#include <WebServer.h>

// Wi-Fi credentials for Access Point mode
const char* ssid = "(Y.B)iPhone";
const char* password = "Furno123";

WebServer server(80);

// Your HTML page as a raw string literal
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang=\"en\">
<head>
    <meta charset=\"UTF-8\">
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
    <title>ESP32 Study Session Scheduler</title>
    <link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css\">
    <style>
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            margin: 0;
            background: linear-gradient(120deg, #e0eafc 0%, #cfdef3 100%);
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
        .lecture-row input[type=\"time\"] {
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
        <form id="scheduleForm">
            <div id="daysContainer"></div>
            <div class="config-section">
                <h2>Configuration</h2>
                <label>Response Window (seconds): <input type="number" id="responseWindow" min="10" max="300" value="30"></label>
                <label>Red LED Warning (minutes before end): <input type="number" id="redLedWarning" min="1" max="30" value="5"></label>
                <label>Red LED Blink (seconds before end): <input type="number" id="redLedBlink" min="5" max="120" value="30"></label>
            </div>
            <button type="submit" class="save-btn"><i class="fa-solid fa-floppy-disk"></i> Save Schedule</button>
        </form>
    </div>
    <footer>
        &copy; 2024 Study Session Scheduler | ESP32 Project
    </footer>
    <script>
        const days = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"];
        const daysContainer = document.getElementById('daysContainer');

        days.forEach(day => {
            const block = document.createElement('div');
            block.className = 'day-block';
            block.innerHTML = `<h2>${day}</h2>
                <div class="lectures" id="lectures-${day}"></div>
                <button type="button" class="add-lecture" onclick="addLecture('${day}')"><i class="fa-solid fa-plus"></i> Add Lecture</button>`;
            daysContainer.appendChild(block);
        });

        function addLecture(day) {
            const lecturesDiv = document.getElementById(`lectures-${day}`);
            const row = document.createElement('div');
            row.className = 'lecture-row';
            row.innerHTML = `
                <label>Start: <input type="time" name="${day}-start[]" required></label>
                <label>End: <input type="time" name="${day}-end[]" required></label>
                <button type="button" class="remove-btn" onclick="this.parentElement.remove()"><i class='fa-solid fa-trash'></i> Remove</button>
            `;
            lecturesDiv.appendChild(row);
        }

        // Expose addLecture to global scope
        window.addLecture = addLecture;

        document.getElementById('scheduleForm').onsubmit = function(e) {
            e.preventDefault();
            // Here you would collect the data and send it to the ESP32
            alert('Schedule saved! (Functionality to be implemented)');
        };
    </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

void setup() {
  Serial.begin(115200);

  // Set up ESP32 as an Access Point
  WiFi.softAP(ssid, password);
  Serial.println();
  Serial.print("Connect to WiFi network: ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
} 