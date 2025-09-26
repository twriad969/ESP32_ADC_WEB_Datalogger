#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <time.h>

// --- Wi-Fi Network Settings ---
const char* ssid = "ESP32_Sensor_Setup";
const char* password = "password123";

// --- Hardware Settings ---
const int LED_PIN = 7; // The pin for the completion indicator LED

// --- Global Variables ---
DNSServer dnsServer;
AsyncWebServer server(80);

bool collectionActive = false;
unsigned long collectionStartMillis;
unsigned long long collectionStartUnix_ms;
const unsigned long collectionDuration = 24 * 60 * 60 * 1000; // 24 hours in milliseconds
String selectedPinsStr = "";

// ADC1 pins available on the ESP32-C3
const int adc1Pins[] = {0, 1, 2, 3, 4};
const int numAdc1Pins = sizeof(adc1Pins) / sizeof(int);

// Arrays to store the calibration parameters for each pin
float slopes[5];
float offsets[5];

// Function to format the timestamp to "HH:MM:SS"
String formatTimestamp(unsigned long long ms) {
  time_t now_sec = ms / 1000;
  struct tm timeinfo;
  gmtime_r(&now_sec, &timeinfo); // Use gmtime for timezone-independent UTC
  char buffer[9]; // HH:MM:SS and the null terminator
  strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
  return String(buffer);
}

// Function to generate the dynamic HTML content
String processor(const String& var) {
  if (var == "PIN_OPTIONS") {
    String options = "";
    for (int i = 0; i < numAdc1Pins; i++) {
      int pin = adc1Pins[i];
      options += "<div style='margin-bottom: 10px; display: flex; align-items: center;'>";
      options += "<input type='checkbox' name='pin' value='" + String(pin) + "' style='width: 20px; height: 20px;'>";
      options += "<label style='margin-left: 10px; font-weight: bold;'>GPIO" + String(pin) + "</label>";
      options += "<span style='margin-left: 20px;'>m:</span><input type='text' name='m" + String(pin) + "' size='5' placeholder='1.0' style='margin-left: 5px;'>";
      options += "<span style='margin-left: 10px;'>b:</span><input type='text' name='b" + String(pin) + "' size='5' placeholder='0.0' style='margin-left: 5px;'>";
      options += "</div>";
    }
    return options;
  }
  if (var == "STATUS") {
    if (collectionActive) {
      unsigned long elapsedMillis = millis() - collectionStartMillis;
      unsigned long remainingMillis = collectionDuration - elapsedMillis;
      int hours = remainingMillis / 3600000;
      int minutes = (remainingMillis % 3600000) / 60000;
      int seconds = (remainingMillis % 60000) / 1000;
      unsigned long long now_ms = collectionStartUnix_ms + elapsedMillis;
      String currentTime = formatTimestamp(now_ms);
      return "<h2>Collection Active!</h2><p>Current Time (UTC): " + currentTime + "</p><p>Time Remaining: " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s</p><a href='/download'><button>Download Current Data</button></a>";
    } else {
      return "<h2>No collection active.</h2><p>The LED on pin " + String(LED_PIN) + " will light up when collection is complete.</p>";
    }
  }
  return String();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  WiFi.softAP(ssid, password);
  Serial.print("Access Point IP: ");
  Serial.println(WiFi.softAPIP());

  dnsServer.start(53, "*", WiFi.softAPIP());

  // --- Web Server Routes ---

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", R"html(
      <!DOCTYPE html><html><head><title>ESP32 Configurator</title><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body{font-family:Arial,sans-serif;margin:20px;background-color:#f4f4f4}.container{max-width:600px;margin:auto;padding:20px;background-color:white;border:1px solid #ccc;border-radius:10px;box-shadow:0 2px 5px rgba(0,0,0,0.1)}button{background-color:#007BFF;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;font-size:16px}h1,h3{color:#333}input[type="text"]{padding:5px;border:1px solid #ccc;border-radius:3px}
      </style></head><body>
      <div class="container"><h1>Data Logger Setup</h1><h3>ADC1 Pins & Calibration (y = mx + b):</h3>
      <form action="/start" method="POST" onsubmit="document.getElementById('startTime').value = new Date().getTime();">
      %PIN_OPTIONS%
      <input type="hidden" id="startTime" name="startTime"><br>
      <button type="submit">Start 24-Hour Collection</button></form><hr>%STATUS%
      </div></body></html>)html", processor);
  });

  server.on("/start", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (collectionActive) { request->send(400, "text/plain", "Collection is already active."); return; }
    if (request->hasParam("startTime", true)) {
      collectionStartUnix_ms = strtoull(request->getParam("startTime", true)->value().c_str(), NULL, 10);
    } else { request->send(400, "text/plain", "Could not get time from browser."); return; }

    // Set default calibration values for all pins before reading parameters
    for (int i = 0; i < numAdc1Pins; i++) {
      slopes[i] = 1.0;
      offsets[i] = 0.0;
    }
    
    selectedPinsStr = "";
    int params = request->params();
    for (int i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      
      // Store the selected pins
      if (p->name() == "pin") {
        if (selectedPinsStr.length() > 0) selectedPinsStr += ",";
        selectedPinsStr += p->value();
      }
      
      // Store the 'm' and 'b' calibration parameters
      if (p->name().startsWith("m")) {
        int pin_idx = p->name().substring(1).toInt();
        if (pin_idx >= 0 && pin_idx < numAdc1Pins && p->value().length() > 0) {
          slopes[pin_idx] = p->value().toFloat();
        }
      }
      if (p->name().startsWith("b")) {
        int pin_idx = p->name().substring(1).toInt();
        if (pin_idx >= 0 && pin_idx < numAdc1Pins && p->value().length() > 0) {
          offsets[pin_idx] = p->value().toFloat();
        }
      }
    }

    if (selectedPinsStr.length() == 0) { request->send(400, "text/plain", "No pins selected."); return; }

    SPIFFS.remove("/data.csv");
    File file = SPIFFS.open("/data.csv", FILE_WRITE);
    if (file) {
      file.print("timestamp,");
      file.println(selectedPinsStr);
      file.close();
    }

    collectionActive = true;
    collectionStartMillis = millis();
    digitalWrite(LED_PIN, LOW);
    request->redirect("/");
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/data.csv", "text/csv", true);
  });
  
  server.onNotFound([](AsyncWebServerRequest *request){ request->redirect("/"); });
  server.begin();
  Serial.println("HTTP Server started");
}

void loop() {
  dnsServer.processNextRequest();

  if (collectionActive) {
    static unsigned long lastReadTime = 0;
    if (millis() - lastReadTime >= 1000) {
      lastReadTime = millis();
      File file = SPIFFS.open("/data.csv", FILE_APPEND);
      if (file) {
        unsigned long long now_ms = collectionStartUnix_ms + (millis() - collectionStartMillis);
        file.print(formatTimestamp(now_ms));
        
        char buf[selectedPinsStr.length() + 1];
        selectedPinsStr.toCharArray(buf, sizeof(buf));
        char* p = strtok(buf, ",");
        while(p != NULL) {
          int pin = atoi(p);
          int adcValue = analogRead(pin);
          
          // Apply the linear calibration function
          // Since the pins are 0-4, the pin number is the same as the array index
          float calibratedValue = (slopes[pin] * adcValue) + offsets[pin];
          
          file.print(",");
          file.print(calibratedValue, 4); // Print the float value with 4 decimal places
          p = strtok(NULL, ",");
        }
        file.println();
        file.close();
      }
    }

    if (millis() - collectionStartMillis >= collectionDuration) {
      collectionActive = false;
      digitalWrite(LED_PIN, HIGH);
      Serial.println("24-hour collection complete! LED is on.");
    }
  }
}
