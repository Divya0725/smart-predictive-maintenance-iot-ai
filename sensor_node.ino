/*
  ============================================================
  Smart Predictive Maintenance System
  ------------------------------------------------------------
  Hardware:
    - ESP32 (or Arduino + ESP8266)
    - MPU6050   → Vibration sensor         (I2C: SDA=21, SCL=22)
    - DHT22     → Temperature sensor       (Digital pin D4)
    - ACS712    → Current sensor           (Analog pin A0)
    - KY-037    → Sound/Noise sensor       (Analog pin A1)
    - OLED 128x64 Display                  (I2C: SDA=21, SCL=22)
  ------------------------------------------------------------
  Author : Divya M
  College: Chennai Institute of Technology
  ============================================================
*/

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_SSD1306.h>

// ─── Wi-Fi Config ─────────────────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASS     = "YOUR_WIFI_PASSWORD";
const char* SERVER_URL    = "http://YOUR_SERVER_IP:5000/data";  // Flask server

// ─── Pin Definitions ─────────────────────────────────────────
#define DHT_PIN       4
#define CURRENT_PIN   A0
#define NOISE_PIN     A1
#define DHT_TYPE      DHT22

// ─── Thresholds (Fault Detection) ────────────────────────────
#define TEMP_MAX        80.0    // °C
#define VIBRATION_MAX   2.5     // g-force
#define CURRENT_MAX     10.0    // Amperes
#define NOISE_MAX       800     // Raw ADC value

// ─── Objects ─────────────────────────────────────────────────
Adafruit_MPU6050 mpu;
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// ─── Sensor Data ─────────────────────────────────────────────
float temperature  = 0;
float humidity     = 0;
float vibration_x  = 0;
float vibration_y  = 0;
float vibration_z  = 0;
float current_val  = 0;
int   noise_val    = 0;
String faultStatus = "NORMAL";

// ─── Setup ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Init OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Predictive Maint.");
  display.println("Initializing...");
  display.display();
  delay(2000);

  // Init MPU6050
  if (!mpu.begin()) {
    Serial.println("[ERROR] MPU6050 not found!");
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("[INFO] MPU6050 ready.");
  }

  // Init DHT
  dht.begin();

  // Connect WiFi
  connectWiFi();

  Serial.println("=== Predictive Maintenance System Started ===");
}

// ─── Main Loop ────────────────────────────────────────────────
void loop() {
  readSensors();
  detectFault();
  displayOnOLED();
  printToSerial();
  sendToServer();
  delay(2000);
}

// ─── Read All Sensors ─────────────────────────────────────────
void readSensors() {
  // Temperature & Humidity
  temperature = dht.readTemperature();
  humidity    = dht.readHumidity();
  if (isnan(temperature)) temperature = 0;

  // Vibration (MPU6050)
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  vibration_x = a.acceleration.x;
  vibration_y = a.acceleration.y;
  vibration_z = a.acceleration.z;

  // Current (ACS712)
  int rawCurrent = analogRead(CURRENT_PIN);
  current_val = ((rawCurrent * 3.3 / 4095.0) - 1.65) / 0.066; // ACS712 30A
  if (current_val < 0) current_val = 0;

  // Noise (KY-037)
  noise_val = analogRead(NOISE_PIN);
}

// ─── Rule-Based Fault Detection ───────────────────────────────
void detectFault() {
  float vibMag = sqrt(vibration_x * vibration_x +
                      vibration_y * vibration_y +
                      vibration_z * vibration_z);

  if (temperature > TEMP_MAX) {
    faultStatus = "OVERHEAT";
  } else if (vibMag > VIBRATION_MAX) {
    faultStatus = "VIBRATION";
  } else if (current_val > CURRENT_MAX) {
    faultStatus = "OVERCURRENT";
  } else if (noise_val > NOISE_MAX) {
    faultStatus = "NOISE";
  } else {
    faultStatus = "NORMAL";
  }
}

// ─── OLED Display ────────────────────────────────────────────
void displayOnOLED() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("-- Machine Status --");
  display.print("Temp  : "); display.print(temperature); display.println(" C");
  display.print("Vibr  : "); display.print(
    sqrt(vibration_x*vibration_x + vibration_y*vibration_y + vibration_z*vibration_z), 2
  ); display.println(" g");
  display.print("Curr  : "); display.print(current_val, 1); display.println(" A");
  display.print("Noise : "); display.println(noise_val);
  display.println("-------------------");
  display.print("Status: "); display.println(faultStatus);
  display.display();
}

// ─── Serial Output ────────────────────────────────────────────
void printToSerial() {
  Serial.println("--- Sensor Readings ---");
  Serial.print("Temperature : "); Serial.print(temperature);  Serial.println(" C");
  Serial.print("Humidity    : "); Serial.print(humidity);     Serial.println(" %");
  Serial.print("Vibration X : "); Serial.println(vibration_x);
  Serial.print("Vibration Y : "); Serial.println(vibration_y);
  Serial.print("Vibration Z : "); Serial.println(vibration_z);
  Serial.print("Current     : "); Serial.print(current_val);  Serial.println(" A");
  Serial.print("Noise       : "); Serial.println(noise_val);
  Serial.print("FAULT STATUS: "); Serial.println(faultStatus);
  Serial.println("-----------------------");
}

// ─── Send to Flask Server ─────────────────────────────────────
void sendToServer() {
  if (WiFi.status() != WL_CONNECTED) { connectWiFi(); return; }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  float vibMag = sqrt(vibration_x*vibration_x + vibration_y*vibration_y + vibration_z*vibration_z);

  String payload = "{";
  payload += "\"temperature\":"  + String(temperature, 1) + ",";
  payload += "\"humidity\":"     + String(humidity, 1)    + ",";
  payload += "\"vibration\":"    + String(vibMag, 3)      + ",";
  payload += "\"current\":"      + String(current_val, 2) + ",";
  payload += "\"noise\":"        + String(noise_val)      + ",";
  payload += "\"fault\":\""      + faultStatus            + "\"";
  payload += "}";

  int responseCode = http.POST(payload);
  Serial.print("[HTTP] Response: "); Serial.println(responseCode);
  http.end();
}

// ─── WiFi Connect ─────────────────────────────────────────────
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] Connecting");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500); Serial.print("."); tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n[WiFi] Connected: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WiFi] Failed — running offline.");
  }
}
