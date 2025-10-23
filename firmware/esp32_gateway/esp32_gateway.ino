#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "";
const char* password = "YOUR_WIFI_PASSWORD";
String apiKey = "0ZSDUEIKIHM7EO7G";
String server = "https://api.thingspeak.com/update";

void setup() {
  Serial.begin(115200);     // UART from Arduino UNO
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void loop() {
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    // Example format from Arduino: S1:512,S2:478,L:310,T:23.1,H:44.2,IRR:1
    int s1 = parseValue(data, "S1");
    int s2 = parseValue(data, "S2");
    int l  = parseValue(data, "L");
    float t = parseValue(data, "T");
    float h = parseValue(data, "H");
    int irr = parseValue(data, "IRR");

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String url = server + "?api_key=" + apiKey +
                   "&field1=" + s1 +
                   "&field2=" + s2 +
                   "&field3=" + t +
                   "&field4=" + h +
                   "&field5=" + l +
                   "&field6=" + irr;
      http.begin(url);
      int code = http.GET();
      Serial.println("HTTP Response: " + String(code));
      http.end();
    }
  }
  delay(15000); // ThingSpeak limit: 15 s minimum between writes
}

int parseValue(String data, String key) {
  int start = data.indexOf(key + ":");
  if (start == -1) return 0;
  start += key.length() + 1;
  int end = data.indexOf(",", start);
  if (end == -1) end = data.length();
  return data.substring(start, end).toInt();
}