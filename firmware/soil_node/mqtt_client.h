#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "secrets.h"

WiFiClient mqttNet;
PubSubClient mqtt(mqttNet);
unsigned long tMqttRetry = 0;

inline void mqttBegin() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(256);
  mqtt.setKeepAlive(30);     // detik; lebih panjang dari lama loop() terblokir HTTPS
  mqtt.setSocketTimeout(3);  // detik
}

// Sambung ke broker bila belum tersambung. Percobaan ulang dibatasi tiap MQTT_RETRY_MS.
// LWT: bila ESP32 putus mendadak, broker sendiri yang mem-publish "offline" (retained).
inline bool mqttEnsure() {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (mqtt.connected()) return true;
  unsigned long now = millis();
  if (tMqttRetry != 0 && now - tMqttRetry < MQTT_RETRY_MS) return false;
  tMqttRetry = now;
  bool ok = mqtt.connect(MQTT_CLIENT_ID, nullptr, nullptr,
                         TOPIC_STATUS, 1, true, "offline");
  if (ok) mqtt.publish(TOPIC_STATUS, "online", true);
  return ok;
}

// Satu pesan JSON berisi semua nilai. NaN (DHT belum terbaca) dikirim sebagai null.
inline bool mqttPublish(int mv, float pct, float t, float h, int light, int st) {
  if (!mqtt.connected()) return false;
  char tb[12], hb[12], msg[200];
  if (isnan(t)) snprintf(tb, sizeof tb, "null"); else snprintf(tb, sizeof tb, "%.1f", t);
  if (isnan(h)) snprintf(hb, sizeof hb, "null"); else snprintf(hb, sizeof hb, "%.1f", h);
  snprintf(msg, sizeof msg,
           "{\"soil_mv\":%d,\"soil_pct\":%.1f,\"temp_c\":%s,\"rh\":%s,"
           "\"light\":%d,\"state\":%d,\"uptime_s\":%lu}",
           mv, pct, tb, hb, light, st, millis() / 1000UL);
  return mqtt.publish(TOPIC_TELEMETRY, msg);
}

// Teks status untuk OLED. Kode negatif = gagal sambung (lihat catatan di bawah).
inline void mqttStatusLine(char* out, size_t n) {
  if (!USE_MQTT)              snprintf(out, n, "MQTT: off");
  else if (mqtt.connected())  snprintf(out, n, "MQTT: OK");
  else                        snprintf(out, n, "MQTT: putus (%d)", mqtt.state());
}