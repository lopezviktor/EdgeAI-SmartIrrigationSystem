#include <Arduino.h>
#include "BluetoothSerial.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"
#include <cstdio>

// UART to Arduino (Hardware Serial1 on ESP32)
HardwareSerial SerialToArduino(1);

// Global Bluetooth Serial instance
BluetoothSerial SerialBT;

// WiFi credentials
const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

// ThingSpeak
const char *TS_API_KEY = THINGSPEAK_KEY;
const char *TS_URL = TS_UPDATE_URL;

// Irrigation decision: 0 = WATER_OFF, 1 = WATER_ON
int decisionFlag = 0;
int wateringSeconds = 0; // irrigation duration received from RPi (0,8,14,18,24)

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

bool g_hasTelemetry = false; // becomes true once we parse at least one UART line from Arduino

// Parse a telemetry line from Arduino in the form:
// "S1:<val>,S2:<val>,T:<val>,H:<val>,L:<val>"
// Returns true if parsing was successful and updates g_state.
bool parseTelemetryLine(const String &line, SensorState &out)
{
  float s1, s2, t, h;
  int l;

  int matched = sscanf(line.c_str(), "S1:%f,S2:%f,T:%f,H:%f,L:%d", &s1, &s2, &t, &h, &l);
  if (matched == 5)
  {
    out.soil1 = s1;
    out.soil2 = s2;
    out.temp = t;
    out.hum = h;
    out.light = l;
    return true;
  }
  return false;
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

  if (!g_hasTelemetry)
  {
    Serial.println("[TS] No telemetry from Arduino yet, skipping upload.");
    return;
  }

  String url = String(TS_URL) + "?api_key=" + TS_API_KEY +
               "&field1=" + String(g_state.soil1) +
               "&field2=" + String(g_state.soil2) +
               "&field3=" + String(g_state.temp) +
               "&field4=" + String(g_state.hum) +
               "&field5=" + String(g_state.light) +
               "&field6=" + String(decisionFlag) +   // 0 o 1
               "&field7=" + String(wateringSeconds); // 0,8,14,18,24

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

  // UART to Arduino: RX = GPIO16, TX = GPIO17, baud 9600 (must match Arduino)
  SerialToArduino.begin(9600, SERIAL_8N1, 16, 17);
  SerialToArduino.setTimeout(50); // small timeout for readStringUntil on UART
  Serial.println("[UART] SerialToArduino started at 9600 baud (RX=16, TX=17).");

  // Bluetooth SPP
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
  // --- Read any incoming telemetry from Arduino over UART, log it and update g_state ---
  while (SerialToArduino.available() > 0)
  {
    String line = SerialToArduino.readStringUntil('\n');
    line.trim();
    if (line.length() == 0)
    {
      continue;
    }

    Serial.print("[UART] From Arduino: ");
    Serial.println(line);

    if (parseTelemetryLine(line, g_state))
    {
      g_hasTelemetry = true;
      Serial.print("[UART] Parsed telemetry -> S1=");
      Serial.print(g_state.soil1);
      Serial.print(", S2=");
      Serial.print(g_state.soil2);
      Serial.print(", T=");
      Serial.print(g_state.temp);
      Serial.print(", H=");
      Serial.print(g_state.hum);
      Serial.print(", L=");
      Serial.println(g_state.light);
    }
    else
    {
      Serial.println("[UART] Failed to parse telemetry line, ignoring.");
    }
  }

  // --- Main logic: send telemetry over Bluetooth, receive decisions, upload to ThingSpeak ---
  static unsigned long lastSend = 0;
  static unsigned long lastTsUpload = 0;

  const unsigned long SEND_INTERVAL_MS = 180000;                // send every 3 minutes
  const unsigned long TS_UPLOAD_INTERVAL = 3UL * 60UL * 1000UL; // upload to ThingSpeak every 3 minutes

  unsigned long now = millis();

  if (now - lastSend >= SEND_INTERVAL_MS)
  {
    lastSend = now;

    if (!g_hasTelemetry)
    {
      // We have not yet received any real telemetry from Arduino; skip sending.
      Serial.println("[ESP32] No telemetry from Arduino yet, skipping BT send.");
    }
    else
    {
      // Build the payload exactly as Raspberry Pi expects, using real sensor data from Arduino
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

    // New format: CMD:WATER_ON;SEC:14
    if (line.length() > 0 && line.startsWith("CMD:"))
    {
      int secIndex = line.indexOf(";SEC:");
      if (secIndex == -1)
      {
        Serial.println("[ESP32] CMD missing ';SEC:'. Ignoring.");
      }
      else
      {
        String cmdPart = line.substring(4, secIndex);  // after "CMD:"
        String secPart = line.substring(secIndex + 5); // after ";SEC:"
        cmdPart.trim();
        secPart.trim();

        int seconds = secPart.toInt();
        wateringSeconds = seconds;

        Serial.print("[ESP32] Parsed CMD: ");
        Serial.print(cmdPart);
        Serial.print(", seconds=");
        Serial.println(seconds);

        // Update decision flag for ThingSpeak
        if (cmdPart == "WATER_ON")
          decisionFlag = 1;
        else if (cmdPart == "WATER_OFF")
          decisionFlag = 0;

        // Forward to Arduino over UART (single-line command)
        String uartMsg = "CMD:" + cmdPart + ";SEC:" + String(seconds) + "\n";
        SerialToArduino.print(uartMsg);
        Serial.print("[UART] Sent to Arduino: ");
        Serial.print(uartMsg);
      }
    }
    // Legacy format: DECISION:WATER_ON
    else if (line.length() > 0 && line.startsWith("DECISION:"))
    {
      String decision = line.substring(9);
      decision.trim();

      Serial.print("[ESP32] Parsed decision (legacy): ");
      Serial.println(decision);

      if (decision == "WATER_ON")
      {
        decisionFlag = 1;
        Serial.println("[ESP32] Legacy WATER_ON received.");
      }
      else if (decision == "WATER_OFF")
      {
        decisionFlag = 0;
        Serial.println("[ESP32] Legacy WATER_OFF received.");
      }
      else
      {
        Serial.println("[ESP32] Unknown legacy decision value received.");
      }

      // Forward legacy decision to Arduino with SEC=0 (keeps Arduino parser simple)
      String uartMsg = "CMD:" + decision + ";SEC:0\n";
      SerialToArduino.print(uartMsg);
      Serial.print("[UART] Sent to Arduino (legacy mapped): ");
      Serial.print(uartMsg);
    }
    else if (line.length() > 0)
    {
      Serial.println("[ESP32] Unknown BT message format. Ignoring.");
    }
  }

  // --- 3) Periodically upload telemetry + decision to ThingSpeak ---

  if (now - lastTsUpload >= TS_UPLOAD_INTERVAL)
  {
    lastTsUpload = now;
    uploadToThingSpeak();
  }
}