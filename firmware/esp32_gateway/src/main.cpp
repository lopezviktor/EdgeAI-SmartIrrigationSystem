#include <Arduino.h>

// UART2 hacia la Raspberry Pi
// RX2 = GPIO16 (recibe lo que transmite la RPi: pin físico 8)
// TX2 = GPIO17 (envía a la RPi: pin físico 10)
HardwareSerial PiUart(2);

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[ESP32] Boot");

  // UART con la Raspberry
  PiUart.begin(9600, SERIAL_8N1, 16, 17);
  delay(200);
  Serial.println("[ESP32] UART2 ready @9600 (GPIO16=RX2, GPIO17=TX2)");
}

void loop() {
  // *** Simulación de lecturas (cámbialas luego por sensores reales) ***
  float soil1 = 45.0;
  float soil2 = 52.0;
  float tempC = 23.5;
  float hum   = 48.0;
  float light = 300.0;

  // Protocolo acordado (5 campos, la Pi calcula soil_max)
  String line = "SOIL1=" + String(soil1, 1) +
                ",SOIL2=" + String(soil2, 1) +
                ",TEMP="  + String(tempC, 1) +
                ",HUM="   + String(hum, 0) +
                ",LIGHT=" + String(light, 0) + "\n";

  // Enviar a la Raspberry
  PiUart.print(line);
  Serial.print("[ESP32→RPi] "); Serial.print(line);

  // Esperar respuesta de la Pi (WATER_ON / WATER_OFF)
  uint32_t t0 = millis();
  String resp;
  while (millis() - t0 < 1500) {           // timeout ~1.5 s
    while (PiUart.available()) {
      char c = (char)PiUart.read();
      if (c == '\n') { t0 = 0xFFFFFFFF; break; }
      resp += c;
    }
    if (t0 == 0xFFFFFFFF) break;
    delay(5);
  }

  if (resp.length()) {
    Serial.print("[RPi→ESP32] "); Serial.println(resp);
  } else {
    Serial.println("[ESP32] (sin respuesta de la RPi)");
  }

  delay(2000);
}