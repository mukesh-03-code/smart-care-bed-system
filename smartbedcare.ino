#include <ESP8266WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#define DHTPIN D4
#define DHTTYPE DHT22
#define PIR_PIN D5
#define IV_PIN D6
#define BUZZER D7
#define PULSE_PIN A0

const char* ssid = "realme";          // 🔹 Your WiFi Name
const char* password = "12345678";    // 🔹 Your WiFi Password
const char* server = "api.thingspeak.com";
String apiKey = "LA3VMGYY30WY07K7";   // 🔹 Your ThingSpeak Write API Key

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

unsigned long previousMillis = 0;
const long uploadInterval = 10000; // 🔹 10 seconds

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Smart Bed Care");
  delay(2000);
  lcd.clear();

  pinMode(PIR_PIN, INPUT);
  pinMode(IV_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  dht.begin();

  // --- WiFi Connection ---
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connecting");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  delay(1500);
  lcd.clear();
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int motion = digitalRead(PIR_PIN);
  int iv = digitalRead(IV_PIN);
  int pulse = analogRead(PULSE_PIN) / 10;  // scaled

  // --- LCD Auto Refresh ---
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp, 1);
  lcd.print(" H:");
  lcd.print(hum, 0);
  lcd.print("%");

  lcd.setCursor(0, 1);
  if (motion == 1) lcd.print("M:Y "); else lcd.print("M:N ");
  if (iv == 1) lcd.print("IV:OK "); else lcd.print("IV:LOW ");
  lcd.print("P:");
  lcd.print(pulse);

  // --- Buzzer Alert for IV LOW ---
  if (iv == 0)
    digitalWrite(BUZZER, HIGH);
  else
    digitalWrite(BUZZER, LOW);

  // --- Serial Monitor ---
  Serial.println("=== Sensor Data ===");
  Serial.print("Temp: "); Serial.println(temp);
  Serial.print("Hum: "); Serial.println(hum);
  Serial.print("Motion: "); Serial.println(motion);
  Serial.print("IV: "); Serial.println(iv);
  Serial.print("Pulse: "); Serial.println(pulse);

  // --- Send to ThingSpeak every 10 seconds ---
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= uploadInterval) {
    previousMillis = currentMillis;
    if (WiFi.status() == WL_CONNECTED) {
      WiFiClient client;
      if (client.connect(server, 80)) {
        String postStr = apiKey;
        postStr += "&field1=" + String(temp);
        postStr += "&field2=" + String(hum);
        postStr += "&field3=" + String(motion);
        postStr += "&field4=" + String(iv);
        postStr += "&field5=" + String(pulse);
        postStr += "\r\n\r\n";

        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(postStr.length());
        client.print("\n\n");
        client.print(postStr);
        client.stop();

        Serial.println("✅ Data sent to ThingSpeak!");
      }
    }
  }

  delay(2000);  // 🔹 LCD refresh rate
}
