#include <Arduino.h>
#include "BluetoothSerial.h"

// Global Bluetooth Serial instance
BluetoothSerial SerialBT;

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
    /* soil1 */ 800.0f,
    /* soil2 */ 820.0f,
    /* temp  */ 21.5f,
    /* hum   */ 70.0f,
    /* light */ 200,
    /* dryPhase */ false,
    /* tick */ 0};

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

void setup()
{
  // USB serial for debugging
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

  Serial.println("Bluetooth SPP started successfully.");
  Serial.println("Device name: SIS-ESP32-GW");
  Serial.println("Waiting for Raspberry Pi to connect over BT...");
}

void loop()
{
  // Update synthetic sensor state (toggle wet/dry every few cycles)
  updateSyntheticSensors();

  // Build a telemetry line similar to Arduino UART format:
  // S1:<value>,S2:<value>,T:<value>,H:<value>,L:<value>
  char buffer[128];
  snprintf(
      buffer,
      sizeof(buffer),
      "S1:%.1f,S2:%.1f,T:%.1f,H:%.1f,L:%d",
      g_state.soil1,
      g_state.soil2,
      g_state.temp,
      g_state.hum,
      g_state.light);

  // Send over Bluetooth to Raspberry Pi
  SerialBT.println(buffer);

  // Also print to USB serial for debugging
  Serial.print("[ESP32] Sent over BT: ");
  Serial.println(buffer);

  delay(3000); // every 3 seconds
}