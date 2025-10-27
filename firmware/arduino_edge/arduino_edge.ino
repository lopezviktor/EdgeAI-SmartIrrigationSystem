// Arduino UNO — Simulated sensor producer (USB Serial for now)
// CSV: SOIL1,SOIL2,TEMP_C,HUMID,LDR,DECISION

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println(F("UNO: simulated producer ready"));
  randomSeed(analogRead(A0));
}

int inferIrrigate(int s1, int s2, float t, float h, int ldr) {
  // Placeholder for TinyML decision
  return (s1 > 600 || s2 > 600) ? 1 : 0;
}

void loop() {
  int soil1 = random(450, 750);
  int soil2 = random(430, 740);
  float temp = random(180, 300) / 10.0;   // 18.0–30.0 ºC
  float hum  = random(350, 750) / 10.0;   // 35–75 %
  int ldr    = random(200, 900);          // raw
  int dec    = inferIrrigate(soil1, soil2, temp, hum, ldr);

  // CSV line
  char buf[96];
  snprintf(buf, sizeof(buf), "%d,%d,%.2f,%.2f,%d,%d\n",
           soil1, soil2, temp, hum, ldr, dec);

  Serial.print(buf);  // for now via USB; later we'll send via UART to ESP32
  delay(5000);
}