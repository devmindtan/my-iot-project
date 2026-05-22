#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char* ssid = "Vo Thuong 2.4G";
const char* password = "01020304";

const char* serverName = "http://192.168.1.239:5000/data";

// ===== DS18B20 =====
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// ===== Device ID =====
String getDeviceId() {
  uint64_t chipid = ESP.getEfuseMac();
  char id[13];
  sprintf(id, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  return String(id);
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());

  sensors.begin();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);

    String deviceId = getDeviceId();

    String json = "{";
    json += "\"device_id\": \"" + deviceId + "\",";
    json += "\"temperature\": " + String(temp);
    json += "}";

    int httpResponseCode = http.POST(json);

    Serial.print("Response: ");
    Serial.println(httpResponseCode);

    Serial.print("Temp: ");
    Serial.println(temp);

    http.end();
  } else {
    Serial.println("WiFi lost!");
  }

  delay(3001);
}
