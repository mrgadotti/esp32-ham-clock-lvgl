#include "wifi_manager.h"

#include <Arduino.h>
#include <WiFi.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "backoff.h"

namespace {
const char* kApSsid = "ESP32-HAM-CLOCK";
const uint32_t kConnectTimeoutMs = 15000;
const uint32_t kApRetryMs = 5UL * 60 * 1000;

enum class St { Init, Connecting, Connected, Backoff, Ap };
St st = St::Init;
uint8_t which = 0;  // 0 = ssid1, 1 = ssid2
uint8_t failures = 0;
uint32_t stamp = 0;
bool apUp = false;
bool retryFromAp = false;
Config c;

void publish() {
  bool sta = WiFi.status() == WL_CONNECTED;
  char ip[16], ssid[33];
  strlcpy(ip, sta ? WiFi.localIP().toString().c_str() : (apUp ? WiFi.softAPIP().toString().c_str() : ""), sizeof ip);
  strlcpy(ssid, sta ? WiFi.SSID().c_str() : (apUp ? kApSsid : ""), sizeof ssid);
  int8_t rssi = sta ? WiFi.RSSI() : 0;
  g_store.net.write([&](NetStatus& n) {
    n.sta = sta;
    n.ap = apUp;
    strlcpy(n.ip, ip, sizeof n.ip);
    strlcpy(n.ssid, ssid, sizeof n.ssid);
    n.rssi = rssi;
  });
}

void startAp() {
  if (strlen(c.ap_pass) >= 8) {
    WiFi.softAP(kApSsid, c.ap_pass);
  } else {
    Serial.println("[WIFI] ap_pass shorter than 8 chars: starting an open AP");
    WiFi.softAP(kApSsid);
  }
  apUp = true;
  Serial.printf("[WIFI] AP %s up, IP %s\n", kApSsid, WiFi.softAPIP().toString().c_str());
}

void stopAp() {
  if (!apUp) return;
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apUp = false;
}

// Starts a STA attempt with the next configured SSID; false if none is configured.
bool beginSta(uint32_t now) {
  for (int i = 0; i < 2; i++, which ^= 1) {
    const char* s = which ? c.wifi_ssid2 : c.wifi_ssid;
    const char* p = which ? c.wifi_pass2 : c.wifi_pass;
    if (!*s) continue;
    WiFi.mode(apUp ? WIFI_AP_STA : WIFI_STA);
    WiFi.setHostname("esp32-ham-clock");
    WiFi.begin(s, p);
    Serial.printf("[WIFI] connecting to %s\n", s);
    stamp = now;
    return true;
  }
  return false;
}
}  // namespace

void wifiTick(uint32_t now) {
  switch (st) {
    case St::Init:
      cfgGet(c);
      WiFi.persistent(false);
      WiFi.setAutoReconnect(false);
      if (beginSta(now)) {
        st = St::Connecting;
      } else {
        startAp();
        stamp = now;
        st = St::Ap;
      }
      break;
    case St::Connecting:
      if (WiFi.status() == WL_CONNECTED) {
        failures = 0;
        retryFromAp = false;
        stopAp();
        Serial.printf("[WIFI] connected, IP %s\n", WiFi.localIP().toString().c_str());
        st = St::Connected;
      } else if (now - stamp > kConnectTimeoutMs) {
        WiFi.disconnect();
        failures++;
        which ^= 1;
        stamp = now;
        // Both SSIDs tried once (or only one configured) without success: fall back to the AP.
        if (retryFromAp || failures >= 2 || !(*c.wifi_ssid2 && *c.wifi_ssid)) {
          if (!apUp) startAp();
          st = St::Ap;
          retryFromAp = false;
          failures = 0;
        } else if (beginSta(now)) {
          // try the other SSID straight away
        }
      }
      break;
    case St::Connected:
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] lost connection");
        failures = 0;
        stamp = now;
        st = St::Backoff;
      }
      break;
    case St::Backoff:
      if (now - stamp >= backoffMs(failures)) {
        cfgGet(c);
        if (beginSta(now)) {
          st = St::Connecting;
        } else {
          startAp();
          stamp = now;
          st = St::Ap;
        }
      }
      break;
    case St::Ap:
      if (now - stamp >= kApRetryMs) {
        cfgGet(c);
        retryFromAp = true;
        if (beginSta(now)) st = St::Connecting;
        else stamp = now;
      }
      break;
  }
  static uint32_t lastPub = 0;
  if (now - lastPub >= 1000) {
    lastPub = now;
    publish();
  }
}
