// ...existing code...
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <WebServer.h>

#ifndef LED_BUILTIN
constexpr uint8_t kLedPin = 2;
#else
constexpr uint8_t kLedPin = LED_BUILTIN;
#endif

constexpr uint16_t kBlinkIntervalMs = 500;

// Custom delay function using millis()
void delayMillis(uint32_t delayTimeMs) {
  uint32_t start = millis();
  while (millis() - start < delayTimeMs) {
    // Optionally yield or do background tasks here
  }
}

constexpr uint8_t DHTPIN = 15; // DATA (out) pin connected to GPIO 15
constexpr uint8_t DHTTYPE = DHT22;
DHT dht(DHTPIN, DHTTYPE);

// WiFi credentials
const char* ssid = "S23 Ultra de Mehdi";
const char* password = "12121212";

WebServer server(80);

float lastTemperature = NAN;
float lastHumidity = NAN;

void handleRoot() {
  String html = "<!DOCTYPE html><html lang=\"fr\">";
  html += "<head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<link rel=\"icon\" href=\"data:,\">";
  html += "<title>ESP32 Dashboard</title>";
  html += "<style>\n";
  html += "body { background: linear-gradient(120deg, #e0eafc 0%, #cfdef3 100%); min-height:100vh; margin:0; font-family: 'Segoe UI', 'Roboto', Arial, sans-serif; display:flex; flex-direction:column; align-items:center; justify-content:center; }\n";
  html += ".card { background: #fff; box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.2); border-radius: 20px; padding: 2rem 2.5rem; min-width: 320px; max-width: 90vw; }\n";
  html += ".title { font-size: 2.2rem; font-weight: 700; color: #0043af; margin-bottom: 1.2rem; letter-spacing: 1px; }\n";
  html += ".sensor-row { display: flex; align-items: center; justify-content: space-between; margin: 1.2rem 0; font-size: 1.3rem; }\n";
  html += ".label { color: #555; font-weight: 500; }\n";
  html += ".value { font-size: 1.5rem; font-weight: 700; color: #2196f3; transition: color 0.3s; }\n";
  html += ".icon { font-size: 2rem; margin-right: 0.7rem; vertical-align: middle; }\n";
  html += "@media (max-width: 500px) { .card { padding: 1rem; min-width: 0; } .title { font-size: 1.3rem; } .sensor-row { font-size: 1rem; } .value { font-size: 1.1rem; } }\n";
  html += "</style>\n";
  html += "<script>\n";
  html += "function animateValue(id, newValue, unit) {\n";
  html += "  const el = document.getElementById(id);\n";
  html += "  if (!el) return;\n";
  html += "  if (el.textContent !== newValue + ' ' + unit && !isNaN(newValue)) {\n";
  html += "    el.style.color = '#43af4a';\n";
  html += "    setTimeout(() => { el.style.color = '#2196f3'; }, 400);\n";
  html += "  }\n";
  html += "  el.textContent = isNaN(newValue) ? 'Erreur' : newValue + ' ' + unit;\n";
  html += "}\n";
  html += "function updateData() {\n";
  html += "  fetch('/data').then(r => r.json()).then(data => {\n";
  html += "    animateValue('temp', data.temperature, '°C');\n";
  html += "    animateValue('hum', data.humidity, '%');\n";
  html += "  });\n";
  html += "}\n";
  html += "setInterval(updateData, 2000);\n";
  html += "window.onload = updateData;\n";
  html += "</script></head>";
  html += "<body>";
  html += "<div class=\"card\">";
  html += "<div class=\"title\">🌡️ Dashboard ESP32 - DHT22</div>";
  html += "<div class=\"sensor-row\"><span class=\"icon\">🌡️</span><span class=\"label\">Température</span><span class=\"value\" id=\"temp\">Chargement...</span></div>";
  html += "<div class=\"sensor-row\"><span class=\"icon\">💧</span><span class=\"label\">Humidité</span><span class=\"value\" id=\"hum\">Chargement...</span></div>";
  html += "</div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"temperature\":";
  json += isnan(lastTemperature) ? "null" : String(lastTemperature);
  json += ",\"humidity\":";
  json += isnan(lastHumidity) ? "null" : String(lastHumidity);
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  const uint32_t serialStart = millis();
  while (!Serial && (millis() - serialStart) < 2000) {
    delay(10);
  }

  Serial.println();
  Serial.println("=== ESP32 Blink diagnostic ===");
  Serial.print("SDK version: ");
  Serial.println(ESP.getSdkVersion());

  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, LOW);

  Serial.print("Using LED pin: ");
  Serial.println(kLedPin);

  // Initialize DHT sensor
  dht.begin();

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up the web server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  static uint32_t lastToggleMs = 0;
  static bool ledIsOn = false;
  const uint32_t now = millis();

  if (now - lastToggleMs >= kBlinkIntervalMs) {
    lastToggleMs = now;
    ledIsOn = !ledIsOn;
    digitalWrite(kLedPin, ledIsOn ? HIGH : LOW);

    Serial.print("LED ");
    Serial.println(ledIsOn ? "ON" : "OFF");
    delayMillis(10);
  }

  // Read humidity and temperature from the sensor
  lastHumidity = dht.readHumidity();
  lastTemperature = dht.readTemperature();

  // Handle web server requests
  server.handleClient();

  delayMillis(2000); // Wait 2 seconds before next read
}