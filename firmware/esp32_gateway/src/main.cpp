#include <Arduino.h>
#include "BluetoothSerial.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"

// Global Bluetooth Serial instance
BluetoothSerial SerialBT;

// WiFi credentials
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// ThingSpeak
const char *TS_API_KEY = "THINGSPEAK_KEY";
const char *TS_URL = "TS_UPDATE_URL";

// Irrigation decision: 0 = WATER_OFF, 1 = WATER_ON
int decisionFlag = 0;

// Simple synthetic sensor model:
// We alternate between a "WET" phase and a "DRY" phase
// so that we can later see different patterns on the Raspberry Pi.
struct SensorState
{
  float soil1;
  float soil2;
  float temp;
  float hum;
  int light;
  bool dryPhase;
  unsigned long tick;
};

SensorState g_state = {
    /* soil1    */ 800.0f,
    /* soil2    */ 820.0f,
    /* temp     */ 21.5f,
    /* hum      */ 70.0f,
    /* light    */ 200,
    /* dryPhase */ false,
    /* tick     */ 0};

// Update the synthetic sensors to simulate "wet" and "dry" situations
void updateSyntheticSensors()
{
  g_state.tick++;

  // Every 10 cycles, toggle between WET and DRY scenario
  if (g_state.tick % 10 == 0)
  {
    g_state.dryPhase = !g_state.dryPhase;
  }

  if (!g_state.dryPhase)
  {
    // WET scenario: soil is moist, temp is lower, humidity higher, light moderate
    g_state.soil1 = 800.0f + (g_state.tick % 10); // small variation
    g_state.soil2 = 820.0f + (g_state.tick % 10);
    g_state.temp = 21.5f + 0.1f * (g_state.tick % 5);
    g_state.hum = 70.0f - 0.2f * (g_state.tick % 5);
    g_state.light = 200 + (g_state.tick % 20);
  }
  else
  {
    // DRY scenario: soil is dry, temp higher, humidity lower, light higher
    g_state.soil1 = 320.0f + (g_state.tick % 10);
    g_state.soil2 = 310.0f + (g_state.tick % 10);
    g_state.temp = 27.0f + 0.1f * (g_state.tick % 5);
    g_state.hum = 40.0f - 0.2f * (g_state.tick % 5);
    g_state.light = 700 + (g_state.tick % 30);
  }
}

// Connect to WiFi if not already connected
void connectWiFiIfNeeded()
{
  if (WiFi.status() == WL_CONNECTED)
    return;

  Serial.println("[WiFi] Connecting...");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("[WiFi] Connected. IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("[WiFi] Failed to connect.");
  }
}

// Upload sensor data and decision to ThingSpeak
void uploadToThingSpeak()
{
  connectWiFiIfNeeded();

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[TS] WiFi not connected, skipping upload");
    return;
  }

  String url = String(TS_URL) + "?api_key=" + TS_API_KEY +
               "&field1=" + String(g_state.soil1) +
               "&field2=" + String(g_state.soil2) +
               "&field3=" + String(g_state.temp) +
               "&field4=" + String(g_state.hum) +
               "&field5=" + String(g_state.light) +
               "&field6=" + String(decisionFlag);

  Serial.println("[TS] Uploading telemetry + decision");
  Serial.println("[TS] URL: " + url);

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.printf("[TS] Response: %d\n", httpCode);
  http.end();
}

void setup()
{
  // USB serial
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== Smart Irrigation Gateway (ESP32, Bluetooth mode) ===");
  Serial.println("Starting Bluetooth SPP...");

  // Start Bluetooth in SPP mode with a recognizable device name
  if (!SerialBT.begin("SIS-ESP32-GW"))
  { // SIS = Smart Irrigation System
    Serial.println("ERROR: Failed to start Bluetooth SPP!");
    // If BT fails, stay here so we see the error
    while (true)
    {
      Serial.println("Bluetooth init failed. Restart the board.");
      delay(3000);
    }
  }

  // Set a timeout for Bluetooth reads so readStringUntil() does not block forever
  SerialBT.setTimeout(200); // 200 ms Bluetooth read timeout

  Serial.println("Bluetooth SPP started successfully.");
  Serial.println("Device name: SIS-ESP32-GW");
  Serial.println("Waiting for Raspberry Pi to connect over BT...");
}

void loop()
{
  static unsigned long lastSend = 0;
  static unsigned long lastTsUpload = 0;

  const unsigned long SEND_INTERVAL_MS = 2000;    // send every 2 seconds
  const unsigned long TS_UPLOAD_INTERVAL = 20000; // upload to ThingSpeak every 20 seconds

  unsigned long now = millis();

  if (now - lastSend >= SEND_INTERVAL_MS)
  {
    lastSend = now;

    // Update synthetic sensor values (no real sensors yet)
    updateSyntheticSensors();

    // Build the payload exactly as Raspberry Pi expects
    String payload = "S1:" + String(g_state.soil1, 1) +
                     ",S2:" + String(g_state.soil2, 1) +
                     ",T:" + String(g_state.temp, 1) +
                     ",H:" + String(g_state.hum, 1) +
                     ",L:" + String(g_state.light);

    // Log to USB Serial (for debugging in Serial Monitor)
    Serial.print("[ESP32] Sent over BT: ");
    Serial.println(payload);

    // Send over Bluetooth to Raspberry Pi (must end with newline)
    SerialBT.print(payload);
    SerialBT.print('\n'); // Ensure newline terminator
    SerialBT.flush();     // Ensure data is pushed to BT stack
  }

  // --- 2) Read decisions sent by Raspberry Pi over Bluetooth SPP ---

  if (SerialBT.available() > 0)
  {
    int availableBytes = SerialBT.available();
    Serial.print("[ESP32] Bytes available from BT: ");
    Serial.println(availableBytes);

    // Read a full line until newline character
    String line = SerialBT.readStringUntil('\n');
    line.trim(); // remove \r, spaces, etc.

    Serial.print("[ESP32] Raw BT line: ");
    Serial.println(line);

    if (line.length() > 0 && line.startsWith("DECISION:"))
    {
      String decision = line.substring(9);
      decision.trim();

      Serial.print("[ESP32] Parsed decision: ");
      Serial.println(decision);

      if (decision == "WATER_ON")
      {
        decisionFlag = 1;
        Serial.println("[ESP32] Activating pump (SIMULATED: WATER_ON).");
        // In the future, this will just forward the command to the Arduino over UART.
      }
      else if (decision == "WATER_OFF")
      {
        decisionFlag = 0;
        Serial.println("[ESP32] Stopping pump (SIMULATED: WATER_OFF).");
      }
      else
      {
        Serial.println("[ESP32] Unknown decision value received.");
      }
    }
    else if (line.length() > 0)
    {
      Serial.println("[ESP32] BT line does not start with 'DECISION:'. Ignoring.");
    }
  }

  // --- 3) Periodically upload telemetry + decision to ThingSpeak ---

  if (now - lastTsUpload >= TS_UPLOAD_INTERVAL)
  {
    lastTsUpload = now;
    uploadToThingSpeak();
  }
}