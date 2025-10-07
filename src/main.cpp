#include <FS.h>
#include <SPIFFS.h>
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
const char* ssid = "Galaxy S25+";
const char* password = "12345678";

WebServer server(80);

float lastTemperature = NAN;
float lastHumidity = NAN;

void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(500, "text/plain", "index.html not found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleStaticFile(const char* path, const char* contentType) {
  File file = SPIFFS.open(path, "r");
  if (!file) {
    server.send(404, "text/plain", String(path) + " not found");
    return;
  }
  server.streamFile(file, contentType);
  file.close();
}
  // Initialize SPIFFS
  void initializeSPIFFS() {
    if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS Mount Failed");
      return;
    }
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
  // Initialize SPIFFS
  initializeSPIFFS();

  // Initialize DHT sensor
  dht.begin();
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
  server.on("/style.css", [](){ handleStaticFile("/style.css", "text/css"); });
  server.on("/script.js", [](){ handleStaticFile("/script.js", "application/javascript"); });
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  static uint32_t lastToggleMs = 0;
  static bool ledIsOn = false;
  static uint32_t lastSensorMs = 0;
  const uint32_t now = millis();

  // Blink LED (unchanged)
  if (now - lastToggleMs >= kBlinkIntervalMs) {
    lastToggleMs = now;
    ledIsOn = !ledIsOn;
    digitalWrite(kLedPin, ledIsOn ? HIGH : LOW);
    Serial.print("LED ");
    Serial.println(ledIsOn ? "ON" : "OFF");
  }

  // Non-blocking sensor read every 2 seconds
  if (now - lastSensorMs >= 2000) {
    lastSensorMs = now;
    lastHumidity = dht.readHumidity();
    lastTemperature = dht.readTemperature();
  }

  // Handle web server requests as fast as possible
  server.handleClient();
}