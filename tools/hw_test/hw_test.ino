// tools/hw_test/hw_test.ino
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

constexpr int PIN_SOIL  = 34;
constexpr int PIN_LIGHT = 35;
constexpr int PIN_DHT   = 4;
constexpr int OLED_SDA  = 21;
constexpr int OLED_SCL  = 22;
constexpr uint8_t OLED_ADDR = 0x3C;

DHT dht(PIN_DHT, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledOk = false;

void scanI2C() {
  Serial.println("--- Scan I2C ---");
  int found = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Perangkat di 0x%02X\n", addr);
      found++;
    }
  }
  if (found == 0) Serial.println("Tidak ada perangkat I2C ditemukan");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(OLED_SDA, OLED_SCL);
  scanI2C();

  oledOk = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  Serial.println(oledOk ? "OLED: OK" : "OLED: GAGAL");

  analogSetPinAttenuation(PIN_SOIL, ADC_11db);
  pinMode(PIN_LIGHT, INPUT);
  dht.begin();
  Serial.println("--- Uji sensor (tiap 2 detik) ---");
}

void loop() {
  int soilMv = analogReadMilliVolts(PIN_SOIL);
  int light  = digitalRead(PIN_LIGHT);
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  Serial.printf("soil=%d mV | light=%d | T=%.1f C | RH=%.0f %%\n", soilMv, light, t, h);

  if (oledOk) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    oled.printf("Soil : %d mV\n", soilMv);
    oled.printf("Light: %d\n", light);
    oled.printf("Suhu : %.1f C\n", t);
    oled.printf("RH   : %.0f %%\n", h);
    oled.display();
  }
  delay(2000);
}
