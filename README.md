# ESP32 Plant Monitor with Database

A smart IoT plant monitoring system built with ESP32 that tracks environmental conditions and stores historical data in a local SQLite database.

## 🌱 Features

- **Real-time Monitoring**: Temperature and humidity readings via DHT22 sensor
- **Visual Feedback**: RGB LED indicators for plant health status
- **Audio Alerts**: Buzzer notifications for critical conditions
- **Web Interface**: Responsive web dashboard accessible via WiFi
- **Database Storage**: SQLite database for historical data tracking
- **REST API**: RESTful endpoints for data access and device control
- **Manual Control**: Physical button and web controls for blue LED

## Components Required

### Hardware
- ESP32 development board (ESP32 DevKit v1)
- DHT22 temperature and humidity sensor
- 4x LEDs (Green, Yellow, Red, Blue) with appropriate resistors
- Buzzer (active)
- Push button
- Breadboard and jumper wires

### Software
- PlatformIO IDE or VS Code with PlatformIO extension
- ESP32 Arduino framework

## Wiring Diagram

| Component | ESP32 Pin | Notes |
|-----------|-----------|-------|
| DHT22 Data | GPIO15 | Temperature & humidity sensor |
| Green LED | GPIO12 | Optimal conditions indicator |
| Yellow LED | GPIO27 | Alert conditions indicator |
| Red LED | GPIO14 | Warning conditions indicator |
| Blue LED | GPIO4 | User controllable |
| Buzzer | GPIO33 | Audio alerts |
| Button | GPIO32 | Manual blue LED control (pull-up) |

**LED Wiring**: LEDs are wired with anode to 3.3V via resistor, cathode to GPIO (active LOW)

## Quick Start

### 1. Setup Development Environment
```bash
# Install VS Code and PlatformIO extension
# Clone or download this project
# Open project folder in VS Code
```

### 2. Configure WiFi
Edit `src/main.cpp` and update your WiFi credentials:
```cpp
const char* ssid = "Your_WiFi_Name";
const char* password = "Your_WiFi_Password";
```

### 3. Build and Upload
- Connect ESP32 via USB
- Press `Ctrl+Shift+P` → "PlatformIO: Upload"
- Open Serial Monitor to view output

### 4. Access Web Interface
- Note the IP address from Serial Monitor
- Open browser: `http://[ESP32_IP_ADDRESS]`

## Plant Health Status

The system evaluates plant conditions based on configurable thresholds:

### Temperature Ranges
- **Optimal**: 18°C - 28°C
- **Warning**: 15°C - 32°C  
- **Alert**: Outside warning range

### Humidity Ranges
- **Optimal**: 40% - 65%
- **Warning**: 30% - 75%
- **Alert**: Outside warning range

### LED Indicators
- 🟢 **Green**: Optimal conditions
- 🟡 **Yellow**: Alert conditions
- 🔴 **Red**: Warning conditions
- 🔵 **Blue**: User controllable

### Audio Alerts
Buzzer activates when:
- Temperature ≥ 35°C
- Humidity ≥ 80%
- Status is "Alert"

## 🌐 API Endpoints

### Current Data
```http
GET /data
```
Returns current sensor readings and device status.

**Response:**
```json
{
  "temperature": 22.5,
  "humidity": 55.2,
  "status": "optimal",
  "blueLed": true,
  "buzzer": false
}
```

### Historical Data
```http
GET /api/historical?limit=100
```
Returns historical sensor readings from database.

**Parameters:**
- `limit` (optional): Number of records (default: 100, max: 1000)

**Response:**
```json
{
  "readings": [
    {
      "timestamp": 1728518400,
      "temperature": 22.5,
      "humidity": 55.2,
      "status": "optimal"
    }
  ],
  "count": 1
}
```

### Blue LED Control
```http
POST /api/blue-led?state=toggle
```
Controls the blue LED state.

**Parameters:**
- `state`: "on", "off", "toggle", "true", "false", "1", "0"

## Database Schema

The system uses SQLite to store sensor readings:

```sql
CREATE TABLE sensor_readings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp INTEGER NOT NULL,
    temperature REAL,
    humidity REAL,
    status TEXT NOT NULL
);
```

Data is automatically saved every 2 seconds when sensor readings are successful.

## 🔧 Configuration

### Sensor Thresholds
Modify these constants in `main.cpp` to adjust plant-specific requirements:

```cpp
// Temperature thresholds (°C)
constexpr float kTempIdealMin = 18.0f;
constexpr float kTempIdealMax = 28.0f;
constexpr float kTempWarningMin = 15.0f;
constexpr float kTempWarningMax = 32.0f;

// Humidity thresholds (%)
constexpr float kHumidityIdealMin = 40.0f;
constexpr float kHumidityIdealMax = 65.0f;
constexpr float kHumidityWarningMin = 30.0f;
constexpr float kHumidityWarningMax = 75.0f;

// Buzzer thresholds
constexpr float kBuzzerTempThreshold = 35.0f;
constexpr float kBuzzerHumidityThreshold = 80.0f;
```

### Time Zone
Update NTP settings for your timezone:
```cpp
const long gmtOffset_sec = 0;        // GMT offset in seconds
const int daylightOffset_sec = 3600; // Daylight saving offset
```

## 🧪 Testing

### API Testing Script
Use the included PowerShell script to test all endpoints:
```powershell
# Update ESP32 IP address in test_api.ps1
.\test_api.ps1
```

### Manual Testing
1. **Hardware**: Verify all LEDs and buzzer respond to environmental changes
2. **Web Interface**: Check real-time data updates and LED control
3. **Database**: Monitor Serial output for "Data inserted successfully"
4. **API**: Test endpoints using browser or API client

## Serial Monitor Output

Normal operation shows:
```
=== ESP32 Plant Monitor ===
WiFi connected
IP address: x.x.x.x
Time synchronized
Database opened successfully
Table created successfully
HTTP server started
Temp: 25.30 C Humidity: 57.60 % Status: optimal
Data inserted successfully
```

## 🔍 Troubleshooting

### Common Issues

**Database errors:**
- Ensure SPIFFS has enough space
- Check for "Table created successfully" in Serial Monitor
- Try erasing flash before upload if issues persist

**WiFi connection fails:**
- Verify WiFi credentials
- Check network availability
- Ensure ESP32 is in range

**Sensor reading NaN:**
- Check DHT22 wiring to GPIO15
- Verify 3.3V power connection
- Allow sensor warm-up time after power-on

**Time synchronization fails:**
- Check internet connectivity
- Verify NTP server accessibility
- Try alternative NTP servers

## 📁 Project Structure

```
TEST/
├── src/
│   └── main.cpp           # Main application code
├── data/
│   ├── index.html         # Web interface
│   ├── style.css          # Styling
│   └── script.js          # Frontend JavaScript
├── platformio.ini         # Project configuration
├── test_api.ps1          # API testing script
└── README.md             # This file
```

##  Dependencies

- ESP32 Arduino Framework
- Adafruit DHT Sensor Library
- Sqlite3Esp32 Library
- Built-in ESP32 libraries (WiFi, WebServer, SPIFFS)


## 📄 License

This project is open source and available under the [MIT License](LICENSE).

