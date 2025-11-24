#include <Arduino.h>
#include <DHT.h>

// --- Pin configuration ---
#define DHTPIN 7
#define DHTTYPE DHT22
#define PUMP_PIN 8 // TIP122 base for pump control
#define SOIL1_PIN A0
#define SOIL2_PIN A1
#define LDR_PIN A2

// --- UART configuration ---
// Serial1: Hardware UART to ESP32 (pins 0 = RX, 1 = TX on Arduino)
#define UART_BAUD_ESP32 9600

DHT dht(DHTPIN, DHTTYPE);

// --- Pump logic ---
// Pump is controlled via TIP122 on the negative side
// HIGH on PUMP_PIN = TIP122 conducts = pump ON.
#define PUMP_ACTIVE_HIGH true

// --- Decision state received from ESP32/Raspberry Pi ---
enum PumpDecision
{
  DECISION_UNKNOWN = -1,
  DECISION_WATER_OFF = 0,
  DECISION_WATER_ON = 1
};

PumpDecision lastDecision = DECISION_UNKNOWN;

// Buffer for incoming UART lines from ESP32
static const size_t DECISION_BUF_SIZE = 64;
char decisionBuf[DECISION_BUF_SIZE];
size_t decisionBufIndex = 0;

// Forward declarations
void sendTelemetryToEsp32(float soil1, float soil2, float temp, float humidity, float light);
void handleIncomingDecision();
void applyDecisionToPump(PumpDecision decision);

void setup()
{
  // USB serial for debugging
  Serial.begin(115200);
  while (!Serial)
  {
    ; // Wait for USB Serial to be ready
  }

  // UART to ESP32
  Serial1.begin(UART_BAUD_ESP32);

  dht.begin();

  pinMode(PUMP_PIN, OUTPUT);
  // Ensure pump is OFF on startup
  if (PUMP_ACTIVE_HIGH)
  {
    digitalWrite(PUMP_PIN, LOW);
  }
  else
  {
    digitalWrite(PUMP_PIN, HIGH);
  }

  Serial.println(F("== Smart Irrigation – Arduino Sensor Node =="));
  Serial.println(F("Role: Read real sensors and send telemetry to ESP32 over UART."));
  Serial.println(F("Decision (WATER_ON/OFF) will come back from ESP32/Raspberry Pi."));
}

void loop()
{
  // 1. Read sensors
  float soil1 = analogRead(SOIL1_PIN);
  float soil2 = analogRead(SOIL2_PIN);
  float light = analogRead(LDR_PIN);
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temp) || isnan(humidity))
  {
    Serial.println(F("[Arduino] DHT read failed, skipping this cycle."));
  }
  else
  {
    // 2. Log locally over USB
    Serial.print(F("[Arduino] S1:"));
    Serial.print(soil1);
    Serial.print(F(", S2:"));
    Serial.print(soil2);
    Serial.print(F(", T:"));
    Serial.print(temp);
    Serial.print(F(", H:"));
    Serial.print(humidity);
    Serial.print(F(", L:"));
    Serial.println(light);

    // 3. Send telemetry to ESP32 over UART (no local TinyML anymore)
    sendTelemetryToEsp32(soil1, soil2, temp, humidity, light);
  }

  // 4. Handle any incoming decision messages from ESP32
  handleIncomingDecision();

  // Small delay for sampling period (2 seconds as before)
  delay(2000);
}

// -------------------------------------------------------------------------
// Send telemetry to ESP32
// Format: S1:<value>,S2:<value>,T:<value>,H:<value>,L:<value>\n
// -------------------------------------------------------------------------
void sendTelemetryToEsp32(float soil1, float soil2, float temp, float humidity, float light)
{
  Serial.println(F("[Arduino→ESP32] Sending telemetry over UART..."));

  Serial1.print(F("S1:"));
  Serial1.print(soil1, 2);
  Serial1.print(F(",S2:"));
  Serial1.print(soil2, 2);
  Serial1.print(F(",T:"));
  Serial1.print(temp, 2);
  Serial1.print(F(",H:"));
  Serial1.print(humidity, 2);
  Serial1.print(F(",L:"));
  Serial1.print(light, 2);
  Serial1.println(); // end of line

  // Also mirror what is sent
  Serial.print(F("[Arduino→ESP32] "));
  Serial.print(F("S1:"));
  Serial.print(soil1, 2);
  Serial.print(F(", S2:"));
  Serial.print(soil2, 2);
  Serial.print(F(", T:"));
  Serial.print(temp, 2);
  Serial.print(F(", H:"));
  Serial.print(humidity, 2);
  Serial.print(F(", L:"));
  Serial.println(light, 2);
}

// -----------------------------------------------------------------
// Handle incoming decision commands from ESP32/Raspberry Pi
// Expected format: "DECISION:WATER_ON" or "DECISION:WATER_OFF"
// ----------------------------------------------------------------
void handleIncomingDecision()
{
  while (Serial1.available() > 0)
  {
    char c = Serial1.read();

    // Protect against buffer overflow
    if (decisionBufIndex < DECISION_BUF_SIZE - 1)
    {
      if (c != '\n' && c != '\r')
      {
        decisionBuf[decisionBufIndex++] = c;
      }
      else
      {
        // End of line received
        decisionBuf[decisionBufIndex] = '\0'; // null-terminate
        decisionBufIndex = 0;

        // Parse the completed line
        Serial.print(F("[Arduino] Received from ESP32: "));
        Serial.println(decisionBuf);

        if (strcmp(decisionBuf, "DECISION:WATER_ON") == 0)
        {
          lastDecision = DECISION_WATER_ON;
          applyDecisionToPump(lastDecision);
        }
        else if (strcmp(decisionBuf, "DECISION:WATER_OFF") == 0)
        {
          lastDecision = DECISION_WATER_OFF;
          applyDecisionToPump(lastDecision);
        }
        else
        {
          Serial.println(F("[Arduino] Unknown message format (ignored)."));
        }
      }
    }
    else
    {
      // Buffer overflow protection: reset the buffer
      decisionBufIndex = 0;
      Serial.println(F("[Arduino] Decision buffer overflow, resetting."));
    }
  }
}

// --------------------------------------------------------------------------
// Apply decision to pump output
// ---------------------------------------------------------------------------
void applyDecisionToPump(PumpDecision decision)
{
  if (decision == DECISION_WATER_ON)
  {
    if (PUMP_ACTIVE_HIGH)
    {
      digitalWrite(PUMP_PIN, HIGH);
    }
    else
    {
      digitalWrite(PUMP_PIN, LOW);
    }
    Serial.println(F("[PUMP] WATER_ON (TIP122 activated)."));
  }
  else if (decision == DECISION_WATER_OFF)
  {
    if (PUMP_ACTIVE_HIGH)
    {
      digitalWrite(PUMP_PIN, LOW);
    }
    else
    {
      digitalWrite(PUMP_PIN, HIGH);
    }
    Serial.println(F("[PUMP] WATER_OFF (TIP122 deactivated)."));
  }
  else
  {
    Serial.println(F("[PUMP] DECISION_UNKNOWN, keeping current state."));
  }
}