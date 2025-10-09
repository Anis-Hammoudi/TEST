<<<<<<< HEAD
#include <FS.h>
#include <SPIFFS.h>
// ...existing code...
=======
>>>>>>> mehdi
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
<<<<<<< HEAD
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
=======
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <sqlite3.h>
#include <SPI.h>
#include <time.h>

constexpr uint8_t kDhtPin = 15;  // DATA pin connected to GPIO15
constexpr uint8_t kDhtType = DHT22;
DHT dht(kDhtPin, kDhtType);

// Status LEDs (active LOW: LED on when pin is driven LOW)
constexpr uint8_t kLedGreenPin = 12;  // D12 -> LED verte (optimal)
constexpr uint8_t kLedYellowPin = 27; // D27 -> LED orange (alert)
constexpr uint8_t kLedRedPin = 14;    // D14 -> LED rouge (warning)
constexpr uint8_t kLedBluePin = 4;

constexpr uint8_t kBuzzerPin = 33;    // active HIGH
constexpr uint8_t kButtonPin = 32;    // button to GND, uses internal pull-up

// Wi-Fi credentials (replace with your own)
>>>>>>> mehdi
const char* ssid = "Galaxy S25+";
const char* password = "12345678";

WebServer server(80);

float lastTemperature = NAN;
float lastHumidity = NAN;

<<<<<<< HEAD
=======
enum class ClimateStatus { Unknown, Optimal, Warning, Alert };
ClimateStatus currentStatus = ClimateStatus::Unknown;
bool blueLedState = true;
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

// Database variables
sqlite3 *db;
const char* dbPath = "/spiffs/sensor_data.db";

// NTP configuration for timestamps
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

void initializeDatabase() {
  // Ensure SPIFFS is mounted first
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed - cannot initialize database");
    return;
  }
  
  Serial.println("Initializing database...");
  int rc = sqlite3_open(dbPath, &db);
  if (rc) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(db));
    db = nullptr;
    return;
  }
  Serial.println("Database opened successfully");

  // Create table if it doesn't exist
  const char* createTableSQL = 
    "CREATE TABLE IF NOT EXISTS sensor_readings ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "timestamp INTEGER NOT NULL, "
    "temperature REAL, "
    "humidity REAL, "
    "status TEXT NOT NULL"
    ");";
  
  char* errMsg = 0;
  rc = sqlite3_exec(db, createTableSQL, 0, 0, &errMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error creating table: %s\n", errMsg);
    sqlite3_free(errMsg);
    sqlite3_close(db);
    db = nullptr;
  } else {
    Serial.println("Table created successfully");
  }
}

void initializeTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Waiting for time synchronization...");
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println("Time synchronized");
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void insertSensorData(float temperature, float humidity, const String& status) {
  if (!db) {
    Serial.println("Database not initialized");
    return;
  }

  time_t now = time(nullptr);
  
  const char* insertSQL = 
    "INSERT INTO sensor_readings (timestamp, temperature, humidity, status) "
    "VALUES (?, ?, ?, ?);";
  
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, insertSQL, -1, &stmt, NULL);
  
  if (rc != SQLITE_OK) {
    Serial.printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return;
  }
  
  sqlite3_bind_int64(stmt, 1, now);
  
  if (isnan(temperature)) {
    sqlite3_bind_null(stmt, 2);
  } else {
    sqlite3_bind_double(stmt, 2, temperature);
  }
  
  if (isnan(humidity)) {
    sqlite3_bind_null(stmt, 3);
  } else {
    sqlite3_bind_double(stmt, 3, humidity);
  }
  
  sqlite3_bind_text(stmt, 4, status.c_str(), -1, SQLITE_STATIC);
  
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.printf("Failed to insert data: %s\n", sqlite3_errmsg(db));
  } else {
    Serial.println("Data inserted successfully");
  }
  
  sqlite3_finalize(stmt);
}

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
  digitalWrite(kLedGreenPin, status == ClimateStatus::Optimal ? HIGH : LOW);
  digitalWrite(kLedYellowPin, status == ClimateStatus::Alert ? HIGH : LOW);                               
  digitalWrite(kLedRedPin, status == ClimateStatus::Warning ? HIGH : LOW);
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

>>>>>>> mehdi
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
<<<<<<< HEAD
  // Initialize SPIFFS
  void initializeSPIFFS() {
    if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS Mount Failed");
      return;
    }
  }
=======
>>>>>>> mehdi

void handleData() {
  String json = "{";
  json += "\"temperature\":";
<<<<<<< HEAD
  json += isnan(lastTemperature) ? "null" : String(lastTemperature);
  json += ",\"humidity\":";
  json += isnan(lastHumidity) ? "null" : String(lastHumidity);
=======
  json += isnan(lastTemperature) ? "null" : String(lastTemperature, 1);
  json += ",\"humidity\":";
  json += isnan(lastHumidity) ? "null" : String(lastHumidity, 1);
  json += ",\"status\":\"";
  json += climateStatusToString(currentStatus);
  json += "\",\"blueLed\":";
  json += blueLedState ? "true" : "false";
  json += ",\"buzzer\":";
  json += buzzerState ? "true" : "false";
>>>>>>> mehdi
  json += "}";
  server.send(200, "application/json", json);
}

<<<<<<< HEAD
=======
void handleHistoricalData() {
  if (!db) {
    server.send(500, "application/json", "{\"error\":\"Database not initialized\"}");
    return;
  }

  // Get limit parameter (default to 100, max 1000)
  int limit = 100;
  if (server.hasArg("limit")) {
    limit = server.arg("limit").toInt();
    if (limit > 1000) limit = 1000;
    if (limit < 1) limit = 1;
  }

  const String query = "SELECT timestamp, temperature, humidity, status FROM sensor_readings ORDER BY timestamp DESC LIMIT " + String(limit);
  
  sqlite3_stmt* stmt;
  int rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL);
  
  if (rc != SQLITE_OK) {
    server.send(500, "application/json", "{\"error\":\"Failed to prepare query\"}");
    return;
  }

  String json = "{\"readings\":[";
  bool first = true;
  
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    if (!first) json += ",";
    first = false;
    
    json += "{";
    json += "\"timestamp\":" + String(sqlite3_column_int64(stmt, 0));
    json += ",\"temperature\":";
    
    if (sqlite3_column_type(stmt, 1) == SQLITE_NULL) {
      json += "null";
    } else {
      json += String(sqlite3_column_double(stmt, 1), 1);
    }
    
    json += ",\"humidity\":";
    if (sqlite3_column_type(stmt, 2) == SQLITE_NULL) {
      json += "null";
    } else {
      json += String(sqlite3_column_double(stmt, 2), 1);
    }
    
    json += ",\"status\":\"" + String((const char*)sqlite3_column_text(stmt, 3)) + "\"";
    json += "}";
  }
  
  json += "],\"count\":" + String(first ? 0 : (limit - (first ? 1 : 0))) + "}";
  
  sqlite3_finalize(stmt);
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

>>>>>>> mehdi
void setup() {
  Serial.begin(115200);
  const uint32_t serialStart = millis();
  while (!Serial && (millis() - serialStart) < 2000) {
    delay(10);
  }

  Serial.println();
<<<<<<< HEAD
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
=======
  Serial.println("=== ESP32 Plant Monitor ===");
  Serial.print("SDK version: ");
  Serial.println(ESP.getSdkVersion());

  setupPins();
  applyBlueLedState();
  applyBuzzerState();
  dht.begin();

  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.println(ssid);
>>>>>>> mehdi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
<<<<<<< HEAD
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up the web server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/style.css", [](){ handleStaticFile("/style.css", "text/css"); });
  server.on("/script.js", [](){ handleStaticFile("/script.js", "application/javascript"); });
=======
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  initializeTime();
  initializeDatabase();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/api/historical", handleHistoricalData);
  server.on("/style.css", []() { handleStaticFile("/style.css", "text/css"); });
  server.on("/script.js",
            []() { handleStaticFile("/script.js", "application/javascript"); });
  server.on("/api/blue-led", HTTP_POST, handleBlueLedCommand);
>>>>>>> mehdi
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
<<<<<<< HEAD
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
=======
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
      
      // Save data to database
      insertSensorData(lastTemperature, lastHumidity, climateStatusToString(currentStatus));
    }

    setStatusLeds(currentStatus);
    updateBuzzerFromReadings();
  }

  server.handleClient();
}
>>>>>>> mehdi
