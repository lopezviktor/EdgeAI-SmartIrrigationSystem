// /firmware/arduino/main.ino
#include <DHT.h>
#define DHTPIN 7
#define DHTTYPE DHT22
#define PUMP_PIN 8

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);   // UART to ESP32
  pinMode(PUMP_PIN, OUTPUT);
  dht.begin();
}

void loop() {
  int soil1 = analogRead(A0);
  int soil2 = analogRead(A1);
  int ldr   = analogRead(A2);
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Threshold demo (ajusta después de calibrar)
  bool irrigate = (soil1 < 500 || soil2 < 500);

  digitalWrite(PUMP_PIN, irrigate ? HIGH : LOW);

  // Telemetry packet to ESP32
  Serial.print("S1:"); Serial.print(soil1);
  Serial.print(",S2:"); Serial.print(soil2);
  Serial.print(",L:");  Serial.print(ldr);
  Serial.print(",T:");  Serial.print(t);
  Serial.print(",H:");  Serial.print(h);
  Serial.print(",IRR:"); Serial.println(irrigate ? 1 : 0);

  delay(2000);
}