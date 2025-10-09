#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>

constexpr uint8_t kDhtPin = 15;  // DATA pin connected to GPIO15
constexpr uint8_t kDhtType = DHT22;
DHT dht(kDhtPin, kDhtType);

// Status LEDs (active LOW: LED on when pin is driven LOW)
constexpr uint8_t kLedGreenPin = 12;  // D12 -> LED verte (optimal)
constexpr uint8_t kLedYellowPin = 14; // D14 -> LED orange (warning)
constexpr uint8_t kLedRedPin = 27;    // D27 -> LED rouge (alert)
constexpr uint8_t kLedBluePin = 13;

constexpr uint8_t kBuzzerPin = 33;    // active HIGH
constexpr uint8_t kButtonPin = 32;    // button to GND, uses internal pull-up

// Wi-Fi credentials (replace with your own)
const char* ssid = "S23 Ultra de Mehdi";
const char* password = "12121212";

WebServer server(80);

float lastTemperature = NAN;
float lastHumidity = NAN;

enum class ClimateStatus { Unknown, Optimal, Warning, Alert };
ClimateStatus currentStatus = ClimateStatus::Unknown;
bool blueLedState = false;
bool buzzerState = false;

// Thresholds for plant comfort (tweak for your plant)
constexpr float kTempIdealMin = 18.0f;
constexpr float kTempIdealMax = 28.0f;
constexpr float kTempWarningMin = 15.0f;
constexpr float kTempWarningMax = 32.0f;

constexpr float kHumidityIdealMin = 40.0f;
constexpr float kHumidityIdealMax = 65.0f;
constexpr float kHumidityWarningMin = 30.0f;
constexpr float kHumidityWarningMax = 75.0f;

constexpr float kBuzzerTempThreshold = 35.0f;
constexpr float kBuzzerHumidityThreshold = 80.0f;
constexpr uint16_t kButtonDebounceMs = 40;

void initializeSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
  }
}

void setupPins() {
  pinMode(kLedGreenPin, OUTPUT);
  pinMode(kLedYellowPin, OUTPUT);
  pinMode(kLedRedPin, OUTPUT);
  pinMode(kLedBluePin, OUTPUT);
  pinMode(kBuzzerPin, OUTPUT);
  pinMode(kButtonPin, INPUT_PULLUP);

  // LEDs are wired anode -> 3V3 via resistor, cathode -> GPIO (active LOW)
  digitalWrite(kLedGreenPin, HIGH);
  digitalWrite(kLedYellowPin, HIGH);
  digitalWrite(kLedRedPin, HIGH);
  digitalWrite(kLedBluePin, HIGH);
  digitalWrite(kBuzzerPin, LOW);
}

void applyBlueLedState() {
  digitalWrite(kLedBluePin, blueLedState ? LOW : HIGH);
}

void applyBuzzerState() {
  digitalWrite(kBuzzerPin, buzzerState ? HIGH : LOW);
}

void setStatusLeds(ClimateStatus status) {
  digitalWrite(kLedGreenPin, status == ClimateStatus::Optimal ? LOW : HIGH);
  digitalWrite(kLedYellowPin, status == ClimateStatus::Warning ? LOW : HIGH);
  digitalWrite(kLedRedPin, status == ClimateStatus::Alert ? LOW : HIGH);
}

void updateBuzzerFromReadings() {
  bool shouldBuzz = false;
  if (!isnan(lastTemperature) && lastTemperature >= kBuzzerTempThreshold) {
    shouldBuzz = true;
  }
  if (!isnan(lastHumidity) && lastHumidity >= kBuzzerHumidityThreshold) {
    shouldBuzz = true;
  }
  if (currentStatus == ClimateStatus::Alert) {
    shouldBuzz = true;
  }
  if (!shouldBuzz && (isnan(lastTemperature) || isnan(lastHumidity))) {
    shouldBuzz = false;
  }

  if (shouldBuzz != buzzerState) {
    buzzerState = shouldBuzz;
    Serial.print("Buzzer ");
    Serial.println(buzzerState ? "ON" : "OFF");
    applyBuzzerState();
  }
}

ClimateStatus evaluateClimate(float temperature, float humidity) {
  if (isnan(temperature) || isnan(humidity)) {
    return ClimateStatus::Unknown;
  }

  const bool tempIdeal = temperature >= kTempIdealMin && temperature <= kTempIdealMax;
  const bool humidityIdeal = humidity >= kHumidityIdealMin && humidity <= kHumidityIdealMax;
  if (tempIdeal && humidityIdeal) {
    return ClimateStatus::Optimal;
  }

  const bool tempWarning =
      temperature >= kTempWarningMin && temperature <= kTempWarningMax;
  const bool humidityWarning =
      humidity >= kHumidityWarningMin && humidity <= kHumidityWarningMax;
  if (tempWarning && humidityWarning) {
    return ClimateStatus::Warning;
  }

  return ClimateStatus::Alert;
}

String climateStatusToString(ClimateStatus status) {
  switch (status) {
    case ClimateStatus::Optimal:
      return "optimal";
    case ClimateStatus::Warning:
      return "warning";
    case ClimateStatus::Alert:
      return "alert";
    default:
      return "unknown";
  }
}

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

void handleData() {
  String json = "{";
  json += "\"temperature\":";
  json += isnan(lastTemperature) ? "null" : String(lastTemperature, 1);
  json += ",\"humidity\":";
  json += isnan(lastHumidity) ? "null" : String(lastHumidity, 1);
  json += ",\"status\":\"";
  json += climateStatusToString(currentStatus);
  json += "\",\"blueLed\":";
  json += blueLedState ? "true" : "false";
  json += ",\"buzzer\":";
  json += buzzerState ? "true" : "false";
  json += "}";
  server.send(200, "application/json", json);
}

void handleBlueLedCommand() {
  if (!server.hasArg("state")) {
    server.send(400, "application/json", "{\"error\":\"missing state\"}");
    return;
  }

  String state = server.arg("state");
  state.toLowerCase();

  if (state == "on" || state == "1" || state == "true") {
    blueLedState = true;
  } else if (state == "off" || state == "0" || state == "false") {
    blueLedState = false;
  } else if (state == "toggle") {
    blueLedState = !blueLedState;
  } else {
    server.send(400, "application/json", "{\"error\":\"invalid state\"}");
    return;
  }

  applyBlueLedState();

  String response = "{\"blueLed\":";
  response += blueLedState ? "true" : "false";
  response += "}";
  server.send(200, "application/json", response);
}

void setup() {
  Serial.begin(115200);
  const uint32_t serialStart = millis();
  while (!Serial && (millis() - serialStart) < 2000) {
    delay(10);
  }

  Serial.println();
  Serial.println("=== ESP32 Plant Monitor ===");
  Serial.print("SDK version: ");
  Serial.println(ESP.getSdkVersion());

  initializeSPIFFS();
  setupPins();
  applyBlueLedState();
  applyBuzzerState();
  dht.begin();

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/style.css", []() { handleStaticFile("/style.css", "text/css"); });
  server.on("/script.js",
            []() { handleStaticFile("/script.js", "application/javascript"); });
  server.on("/api/blue-led", HTTP_POST, handleBlueLedCommand);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  static uint32_t lastSensorMs = 0;
  static uint32_t lastButtonChangeMs = 0;
  static int lastButtonReading = HIGH;
  static int stableButtonState = HIGH;

  const uint32_t now = millis();

  // Handle physical button with simple debounce
  const int reading = digitalRead(kButtonPin);
  if (reading != lastButtonReading) {
    lastButtonChangeMs = now;
    lastButtonReading = reading;
  }

  if ((now - lastButtonChangeMs) > kButtonDebounceMs && reading != stableButtonState) {
    stableButtonState = reading;
    if (stableButtonState == LOW) {
      blueLedState = !blueLedState;
      applyBlueLedState();
      Serial.print("Blue LED (button) -> ");
      Serial.println(blueLedState ? "ON" : "OFF");
    }
  }

  if (now - lastSensorMs >= 2000) {
    lastSensorMs = now;
    const float humidity = dht.readHumidity();
    const float temperature = dht.readTemperature();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Sensor read failed");
      currentStatus = ClimateStatus::Unknown;
      lastTemperature = NAN;
      lastHumidity = NAN;
    } else {
      lastTemperature = temperature;
      lastHumidity = humidity;
      currentStatus = evaluateClimate(lastTemperature, lastHumidity);
      Serial.print("Temp: ");
      Serial.print(lastTemperature);
      Serial.print(" C Humidity: ");
      Serial.print(lastHumidity);
      Serial.print(" % Status: ");
      Serial.println(climateStatusToString(currentStatus));
    }

    setStatusLeds(currentStatus);
    updateBuzzerFromReadings();
  }

  server.handleClient();
}
