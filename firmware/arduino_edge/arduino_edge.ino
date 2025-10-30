#include <Arduino.h>
#include <DHT.h>
#include "predict_need_water.h"
#include "scaler.h"

// --- Pin configuration ---
#define DHTPIN 7
#define DHTTYPE DHT22
#define PUMP_PIN 8
#define SOIL1_PIN A0
#define SOIL2_PIN A1
#define LDR_PIN   A2

DHT dht(DHTPIN, DHTTYPE);

// --- Relay logic ---
#define PUMP_ACTIVE_HIGH true  // set false if relay is active LOW

void setup() {
  Serial.begin(115200);   // UART to ESP32 or serial monitor
  dht.begin();

  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, PUMP_ACTIVE_HIGH ? LOW : HIGH); // pump OFF

  Serial.println(F("Edge AI irrigation system ready."));
}

void loop() {
  float soil1 = analogRead(SOIL1_PIN);
  float soil2 = analogRead(SOIL2_PIN);
  float light = analogRead(LDR_PIN);
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temp) || isnan(humidity)) {
    Serial.println(F("DHT read failed, skipping cycle"));
    delay(2000);
    return;
  }

  // Normalize features
  float raw[5] = { soil1, soil2, temp, humidity, light };
  float scaled[5] = { 0, 0, 0, 0, 0 };
  scale_features(raw, scaled);

  Serial.print("Scaled: ");
  for (int i = 0; i < 5; ++i) { Serial.print(scaled[i], 3); Serial.print(i<4?",":"\n"); }
  // Predict irrigation need (0 = no, 1 = yes)
  int pred = predict_need_water(scaled);

  // Control pump
  bool irrigate = (pred == 1);
  if (PUMP_ACTIVE_HIGH)
    digitalWrite(PUMP_PIN, irrigate ? HIGH : LOW);
  else
    digitalWrite(PUMP_PIN, irrigate ? LOW : HIGH);

  // Send telemetry to ESP32
  Serial.print("S1:"); Serial.print(soil1);
  Serial.print(",S2:"); Serial.print(soil2);
  Serial.print(",T:");  Serial.print(temp);
  Serial.print(",H:");  Serial.print(humidity);
  Serial.print(",L:");  Serial.print(light);
  Serial.print(",PRED:"); Serial.println(pred);

  delay(2000); // sampling every 2s
}