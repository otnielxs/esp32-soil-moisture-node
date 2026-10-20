#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include "config.h"
#include "secrets.h"

// ---------- Sensor tanah: median N sampel + konversi kalibrasi ----------
class SoilSensor {
 public:
  explicit SoilSensor(int pin) : pin_(pin) {}

  void begin() { analogSetPinAttenuation(pin_, ADC_11db); }

  int readMv() const {
    int v[N_SAMPLES];
    for (int i = 0; i < N_SAMPLES; i++) {
      v[i] = analogReadMilliVolts(pin_);
      delay(5);
    }
    for (int i = 1; i < N_SAMPLES; i++) {  // insertion sort
      int key = v[i];
      int j = i - 1;
      while (j >= 0 && v[j] > key) {
        v[j + 1] = v[j];
        j--;
      }
      v[j + 1] = key;
    }
    return v[N_SAMPLES / 2];
  }

  static float toPercent(int mv) {
    if (mv >= CAL_MV[0]) return CAL_PCT[0];            // lebih kering dari titik 1
    for (int i = 1; i < CAL_N; i++) {
      if (mv >= CAL_MV[i]) {
        float frac = (CAL_MV[i - 1] - mv) / (CAL_MV[i - 1] - CAL_MV[i]);
        return CAL_PCT[i - 1] + frac * (CAL_PCT[i] - CAL_PCT[i - 1]);
      }
    }
    return CAL_PCT[CAL_N - 1];                         // lebih basah dari titik terakhir: jenuh
  }

 private:
  int pin_;
};

// ---------- Logika keputusan penyiraman (tanpa aktuator) ----------
enum class State { IDLE, NEED_WATER, COOLDOWN };

const char* stateName(State s) {
  switch (s) {
    case State::IDLE:       return "IDLE";
    case State::NEED_WATER: return "SIRAM!";
    case State::COOLDOWN:   return "COOLDOWN";
  }
  return "?";
}

SoilSensor soil(PIN_SOIL);
DHT dht(PIN_DHT, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

State state = State::IDLE;
unsigned long cooldownStart = 0;
float lastT = NAN, lastH = NAN;
unsigned long tSample = 0, tDht = 0, tUpload = 0, tWifi = 0;

void updateState(float pct) {
  switch (state) {
    case State::IDLE:
      if (pct < PCT_LOW) state = State::NEED_WATER;
      break;
    case State::NEED_WATER:
      if (pct > PCT_HIGH) {
        state = State::COOLDOWN;
        cooldownStart = millis();
      }
      break;
    case State::COOLDOWN:
      if (millis() - cooldownStart > COOLDOWN_MS) state = State::IDLE;
      break;
  }
}

// ---------- WiFi dan Adafruit IO ----------
void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  if (tWifi != 0 && millis() - tWifi < WIFI_RETRY_MS) return;
  tWifi = millis();
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

bool postFeed(const char* key, float value) {
  if (isnan(value)) return false;
  WiFiClientSecure client;
  client.setInsecure();  // hanya proyek hobi: sertifikat server tidak diverifikasi
  HTTPClient http;
  http.setTimeout(5000);
  String url = String("https://io.adafruit.com/api/v2/") + AIO_USER + "/feeds/" + key + "/data";
  if (!http.begin(client, url)) return false;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-AIO-Key", AIO_KEY);
  int code = http.POST(String("{\"value\":") + String(value, 2) + "}");
  http.end();
  return code == 200;
}

void upload(int mv, float pct, int light) {
  if (WiFi.status() != WL_CONNECTED) return;
  postFeed("soil-mv", mv);
  postFeed("soil-pct", pct);
  postFeed("light", light);
  postFeed("air-temp", lastT);
  postFeed("air-rh", lastH);
  postFeed("state", (int)state);
}

// ---------- OLED ----------
void drawOled(int mv, float pct, int light) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.printf("Tanah: %.0f %% (%d mV)\n", pct, mv);
  oled.printf("Udara: %.1f C  %.0f %%\n", lastT, lastH);
  oled.printf("Cahaya (0/1): %d\n", light);
  oled.printf("Status: %s\n", stateName(state));
  oled.printf("WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "OK" : "putus");
  oled.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED tidak ditemukan");
  }
  soil.begin();
  pinMode(PIN_LIGHT, INPUT);          // modul LDR pin DO: digital 0/1
  if (USE_DHT) dht.begin();
  WiFi.mode(WIFI_STA);
  Serial.println("ms,soil_mv,soil_pct,temp_c,rh,light_raw,state");
}

void loop() {
  unsigned long now = millis();
  ensureWifi();

  if (USE_DHT && now - tDht >= DHT_MS) {
    tDht = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) lastT = t;
    if (!isnan(h)) lastH = h;
  }

  if (now - tSample >= SAMPLE_MS) {
    tSample = now;
    int mv = soil.readMv();
    float pct = SoilSensor::toPercent(mv);
    int light = digitalRead(PIN_LIGHT);  // 0/1 (modul DO)
    updateState(pct);

    Serial.printf("%lu,%d,%.1f,%.1f,%.1f,%d,%d\n", now, mv, pct, lastT, lastH, light, (int)state);
    drawOled(mv, pct, light);

    if (now - tUpload >= UPLOAD_MS) {
      tUpload = now;
      upload(mv, pct, light);
    }
  }
}
