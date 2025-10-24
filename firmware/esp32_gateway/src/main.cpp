#include <Arduino.h>
#include <secrets.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ==== Config ==== 
#define UART_BAUD       115200
#define SEND_PERIOD_MS  16000
#define ENABLE_UART     false

// ==== Utils ====

int parseIntValue (const String& data, const String& key) {
  int start = data.indexOf(key + ":"); if (start == -1) return 0;
  start += key.length() + 1;
  int end = data.indexOf(",", start); if (end == -1) end = data.length();
  return data.substring(start, end).toInt();
}

float parseFloatValue(const String& data, const String& key) {
  int start = data.indexOf(key + ":"); if (start == -1) return 0.0;
  start += key.length() + 1;
  int end = data.indexOf(",", start); if (end == -1) end = data.length();
  return data.substring(start, end).toFloat();
}

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 60) { 
    delay(500); Serial.print(".");
    retries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection FAILED");
  }
}

bool sendToThingSpeak(int s1, int s2, int l, float t, float h, int irr) {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = String(TS_UPDATE_URL) +
               "?api_key=" + THINGSPEAK_KEY +
               "&field1=" + s1 +
               "&field2=" + s2 +
               "&field3=" + String(t, 2) +
               "&field4=" + String(h, 2) +
               "&field5=" + l +
               "&field6=" + irr;

  http.begin(url);
  int code = http.GET();
  Serial.print("HTTP Response: "); Serial.println(code);
  http.end();
  return (code == 200);
}

void setup() {
  Serial.begin(UART_BAUD);   
  Serial.setTimeout(100);
  delay(300);
  connectWiFi();
}

void loop() {
  int s1, s2, l, irr; float t, h;

  bool gotUart = false;
#if ENABLE_UART
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    if (line.length() > 0) {
      s1  = parseIntValue(line, "S1");
      s2  = parseIntValue(line, "S2");
      l   = parseIntValue(line, "L");
      t   = parseFloatValue(line, "T");
      h   = parseFloatValue(line, "H");
      irr = parseIntValue(line, "IRR");
      gotUart = true;
      Serial.println("UART line: " + line);
    }
  }
#endif

  if (!gotUart) {
    s1 = 450; s2 = 520; l = 300; t = 22.8; h = 46.5; irr = 1;
  }

  bool ok = sendToThingSpeak(s1, s2, l, t, h, irr);
  if (!ok) Serial.println("Send failed (no 200).");

  delay(SEND_PERIOD_MS);
}