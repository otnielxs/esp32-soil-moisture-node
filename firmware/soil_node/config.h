#pragma once
#include <Arduino.h>

// Pin
constexpr int PIN_SOIL  = 34;
constexpr int PIN_LIGHT = 35;   // modul LDR, pin DO (digital 0/1)
constexpr int PIN_DHT   = 4;
constexpr int OLED_SDA  = 21;
constexpr int OLED_SCL  = 22;
constexpr uint8_t OLED_ADDR = 0x3C;

// Set false kalau DHT11 tidak terbaca stabil
constexpr bool USE_DHT = true;

// Pembacaan sensor
constexpr int N_SAMPLES = 15;


// Tabel kalibrasi (hasil timbangan + sensor). Urut dari mV terbesar (kering).
// Titik 32,7 % dibuang: data rusak. Rentang 0-22 % akurat, 22-43 % kasar.
constexpr int   CAL_N = 4;
constexpr float CAL_MV[CAL_N]  = {2107.3f, 1216.8f, 966.1f, 867.1f};
constexpr float CAL_PCT[CAL_N] = {0.0f, 10.9f, 22.3f, 43.3f};

// Logika keputusan (histeresis)
constexpr float PCT_LOW  = 10.0f;   // di bawah ini -> perlu disiram
constexpr float PCT_HIGH = 18.0f;   // di atas ini -> dianggap sudah disiram
constexpr unsigned long COOLDOWN_MS = 10UL * 60UL * 1000UL;

// Penjadwalan
constexpr unsigned long SAMPLE_MS     = 1000;
constexpr unsigned long DHT_MS        = 5000;
constexpr unsigned long UPLOAD_MS     = 60000;
constexpr unsigned long WIFI_RETRY_MS = 30000;
