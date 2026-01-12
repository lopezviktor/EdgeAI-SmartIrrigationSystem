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

// Seconds requested by the controller (RPi) for the current command.
// We keep it for logging / later actuation enablement.
int lastDoseSeconds = 0;

// Safety switch: keep false for now (log-only mode).
// When we are ready, we can set this true to enable timed actuation.

bool ACTUATION_ENABLED = true;

// --- Timed actuation state (non-blocking) ---
// When enabled, WATER_ON + SEC will start the pump for SEC seconds.
// WATER_OFF will stop immediately.
bool pumpRunning = false;
unsigned long pumpStopAtMs = 0;

// Safety: clamp maximum seconds to prevent runaway watering.
const int MAX_PUMP_SECONDS = 30;

// Buffer for incoming UART lines from ESP32
static const size_t DECISION_BUF_SIZE = 64;
char decisionBuf[DECISION_BUF_SIZE];
size_t decisionBufIndex = 0;

// --- Sampling interval (3 minutes) ---
const unsigned long SAMPLE_INTERVAL_MS = 3UL * 60UL * 1000UL; // 180000 ms
unsigned long lastSampleMs = 0;

// --- Manual watering durations (ms) ---
const unsigned long PUMP_LOW_MS = 8000;     // ~Low dose (real test)
const unsigned long PUMP_MEDIUM_MS = 14000; // ~Medium dose (target)
const unsigned long PUMP_HIGH_MS = 18000;   // ~High dose (target)

// Forward declarations
void sendTelemetryToEsp32(float soil1, float soil2, float temp, float humidity, float light);
void handleIncomingDecision();
void applyDecisionToPump(PumpDecision decision);
void handleManualPumpCommands();

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
  Serial.println(F("Manual pump commands over USB: PUMP_LOW, PUMP_MEDIUM, PUMP_HIGH"));
}

void loop()
{
  unsigned long now = millis();

  // Check for manual pump commands from USB Serial (non-blocking when idle)
  handleManualPumpCommands();

  // Non-blocking timed pump stop
  if (ACTUATION_ENABLED && pumpRunning)
  {
    if ((long)(millis() - pumpStopAtMs) >= 0)
    {
      applyDecisionToPump(DECISION_WATER_OFF);
      pumpRunning = false;
      Serial.println(F("[PUMP] Timed watering finished (auto-off)."));
    }
  }

  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS)
  {
    lastSampleMs = now;

    // 1.1 Read sensors
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
      // 1.2 Log locally over USB
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

      // 1.3 Send telemetry to ESP32 over UART
      sendTelemetryToEsp32(soil1, soil2, temp, humidity, light);
    }
  }

  handleIncomingDecision();
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

// --------------------------------------------------------------------------
// Handle manual pump commands over USB Serial
// Commands (send from Serial Monitor, newline-terminated):
//   PUMP_LOW
//   PUMP_MEDIUM
//   PUMP_HIGH
// --------------------------------------------------------------------------
void handleManualPumpCommands()
{
  if (Serial.available() > 0)
  {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); // remove \r and spaces

    if (cmd.equalsIgnoreCase("PUMP_LOW"))
    {
      Serial.println(F("[PUMP] Manual LOW watering (~8 s)"));
      applyDecisionToPump(DECISION_WATER_ON);
      delay(PUMP_LOW_MS);
      applyDecisionToPump(DECISION_WATER_OFF);
      Serial.println(F("[PUMP] Manual LOW watering finished"));
    }
    else if (cmd.equalsIgnoreCase("PUMP_MEDIUM"))
    {
      Serial.println(F("[PUMP] Manual MEDIUM watering (~14 s)"));
      applyDecisionToPump(DECISION_WATER_ON);
      delay(PUMP_MEDIUM_MS);
      applyDecisionToPump(DECISION_WATER_OFF);
      Serial.println(F("[PUMP] Manual MEDIUM watering finished"));
    }
    else if (cmd.equalsIgnoreCase("PUMP_HIGH"))
    {
      Serial.println(F("[PUMP] Manual HIGH watering (~18 s)"));
      applyDecisionToPump(DECISION_WATER_ON);
      delay(PUMP_HIGH_MS);
      applyDecisionToPump(DECISION_WATER_OFF);
      Serial.println(F("[PUMP] Manual HIGH watering finished"));
    }
    else
    {
      Serial.print(F("[PUMP] Unknown manual command: "));
      Serial.println(cmd);
    }
  }
}

// -----------------------------------------------------------------
// Handle incoming decision commands from ESP32/Raspberry Pi
// Expected formats (newline-terminated):
//   CMD:WATER_ON;SEC:14
//   CMD:WATER_OFF;SEC:0
// Legacy (still supported):
//   DECISION:WATER_ON
//   DECISION:WATER_OFF
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

        String line = String(decisionBuf);
        line.trim();

        // New format: CMD:WATER_ON;SEC:14
        if (line.startsWith("CMD:"))
        {
          int secIdx = line.indexOf(";SEC:");
          if (secIdx == -1)
          {
            Serial.println(F("[Arduino] CMD missing ';SEC:' (ignored)."));
          }
          else
          {
            String cmdPart = line.substring(4, secIdx);  // after "CMD:"
            String secPart = line.substring(secIdx + 5); // after ";SEC:"
            cmdPart.trim();
            secPart.trim();

            lastDoseSeconds = secPart.toInt();

            if (cmdPart == "WATER_ON")
            {
              lastDecision = DECISION_WATER_ON;
            }
            else if (cmdPart == "WATER_OFF")
            {
              lastDecision = DECISION_WATER_OFF;
            }
            else
            {
              lastDecision = DECISION_UNKNOWN;
            }

            Serial.print(F("[Arduino] Parsed CMD -> decision="));
            Serial.print(cmdPart);
            Serial.print(F(", seconds="));
            Serial.println(lastDoseSeconds);

            // Optional actuation (disabled by default)
            if (ACTUATION_ENABLED)
            {
              // Clamp seconds for safety
              int sec = lastDoseSeconds;
              if (sec < 0)
                sec = 0;
              if (sec > MAX_PUMP_SECONDS)
                sec = MAX_PUMP_SECONDS;

              if (lastDecision == DECISION_WATER_ON)
              {
                if (sec == 0)
                {
                  Serial.println(F("[PUMP] WATER_ON received with SEC=0 (ignored)."));
                }
                else
                {
                  // Start / refresh timed watering
                  applyDecisionToPump(DECISION_WATER_ON);
                  pumpRunning = true;
                  pumpStopAtMs = millis() + (unsigned long)sec * 1000UL;

                  Serial.print(F("[PUMP] Timed watering started for "));
                  Serial.print(sec);
                  Serial.println(F(" s"));
                }
              }
              else if (lastDecision == DECISION_WATER_OFF)
              {
                // Stop immediately
                applyDecisionToPump(DECISION_WATER_OFF);
                pumpRunning = false;
                Serial.println(F("[PUMP] WATER_OFF received (immediate stop)."));
              }
            }
          }
        }
        // Legacy format
        else if (line == "DECISION:WATER_ON")
        {
          lastDecision = DECISION_WATER_ON;
          lastDoseSeconds = 0;
          Serial.println(F("[Arduino] Legacy decision WATER_ON received."));
          if (ACTUATION_ENABLED)
            applyDecisionToPump(lastDecision);
        }
        else if (line == "DECISION:WATER_OFF")
        {
          lastDecision = DECISION_WATER_OFF;
          lastDoseSeconds = 0;
          Serial.println(F("[Arduino] Legacy decision WATER_OFF received."));
          if (ACTUATION_ENABLED)
          {
            applyDecisionToPump(lastDecision);
            pumpRunning = false;
          }
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