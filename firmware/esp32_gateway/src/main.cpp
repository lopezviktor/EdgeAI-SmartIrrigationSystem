#include <Arduino.h>
#include <secrets.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ==== Config ==== 
#define UART_BAUD       115200
#define SEND_PERIOD_MS  16000
#define ENABLE_UART     false

#define UART_RX_PIN     16
#define UART_TX_PIN     17
HardwareSerial UART2(2);

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

bool parseCsvLine(const String& line, int& s1, int& s2, int& l, float& t, float& h, int& irr) {
  int part = 0, last = 0;
  String p[6];
  for (int i = 0; i < line.length(); ++i) {
    char c = line[i];
    if (c == ',' || c == '\n' || c == '\r') {
      if (part < 6) p[part++] = line.substring(last, i);
      last = i + 1;
    }
  }
  if (last < line.length() && part < 6) p[part++] = line.substring(last);
  if (part != 6) return false;

  s1 = p[0].toInt();
  s2 = p[1].toInt();
  t  = p[2].toFloat();
  h  = p[3].toFloat();
  l  = p[4].toInt();
  irr = p[5].toInt();
  return true;
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
  UART2.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
  randomSeed(esp_random());
  Serial.setTimeout(100);
  delay(300);
  connectWiFi();
}

void loop() {
  int s1, s2, l, irr; float t, h;

  bool gotUart = false;
#if ENABLE_UART
  if (UART2.available()) {
    String line = UART2.readStringUntil('\n');
    if (line.length() > 0) {
      if (parseCsvLine(line, s1, s2, l, t, h, irr)) {
        gotUart = true;
        Serial.println("UART CSV: " + line);
      } else {
        Serial.println("CSV parse failed: " + line);
      }
    }
  }
#endif

if (!gotUart) {
  s1 = random(450, 750);
  s2 = random(430, 740);
  l  = random(200, 900);
  t  = random(180, 300) / 10.0;   // 18.0–30.0 ºC
  h  = random(350, 750) / 10.0;   // 35–75 %
  irr = (s1 > 600 || s2 > 600) ? 1 : 0;
}

  bool ok = sendToThingSpeak(s1, s2, l, t, h, irr);
  if (!ok) UART2.println("Send failed (no 200).");

  delay(SEND_PERIOD_MS);
}