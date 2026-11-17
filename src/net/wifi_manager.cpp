#include "wifi_manager.h"
#include "app/boot_log.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <lwip/dns.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "backoff.h"

namespace {
const char* kApSsid = "ESP32-HAM-CLOCK";
const uint32_t kConnectTimeoutMs = 40000;  // real board sometimes needs >15 s to join
const uint32_t kApRetryMs = 2UL * 60 * 1000;

enum class St { Init, Connecting, Connected, Backoff, Ap };
St st = St::Init;
uint8_t which = 0;  // 0 = ssid1, 1 = ssid2
uint8_t failures = 0;     // failed attempts in the current round (2 -> AP)
uint8_t staFailures = 0;  // failed rounds since the last successful connect; drives backoffMs
bool wasConnected = false;
uint32_t stamp = 0;
bool apUp = false;
bool retryFromAp = false;
Config c;

// Runs every second: no String temporaries (IPAddress::toString, WiFi.SSID) so the net task does not churn the heap
// next to TLS. IP/SSID text is rebuilt only when the link or address changes.
void publish() {
  static bool lastSta = false, lastAp = false;
  static uint32_t lastAddr = 0;
  static char ip[16] = "", ssid[33] = "";
  bool sta = WiFi.status() == WL_CONNECTED;
  uint32_t addr = sta ? (uint32_t)WiFi.localIP() : apUp ? (uint32_t)WiFi.softAPIP() : 0;
  if (sta != lastSta || apUp != lastAp || addr != lastAddr) {
    lastSta = sta;
    lastAp = apUp;
    lastAddr = addr;
    snprintf(ip, sizeof ip, "%u.%u.%u.%u", (unsigned)(addr & 255), (unsigned)(addr >> 8 & 255),
             (unsigned)(addr >> 16 & 255), (unsigned)(addr >> 24));
    if (!addr) ip[0] = 0;
    wifi_config_t wc;
    if (sta && esp_wifi_get_config(WIFI_IF_STA, &wc) == ESP_OK) {
      memcpy(ssid, wc.sta.ssid, 32);  // sta.ssid has no NUL for a 32-char SSID
      ssid[32] = 0;
    } else {
      strlcpy(ssid, apUp && !sta ? kApSsid : "", sizeof ssid);
    }
  }
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
  bootLog("AP fallback %s", WiFi.softAPIP().toString().c_str());
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
    WiFi.setAutoReconnect(false);
    WiFi.setHostname("esp32-ham-clock");
    WiFi.begin(s, p);
    Serial.printf("[WIFI] connecting to %s\n", s);
    bootLog("WiFi connecting to %s", s);
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
      failures = 0;
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
        staFailures = 0;
        wasConnected = true;
        retryFromAp = false;
        stopAp();
        Serial.printf("[WIFI] connected, IP %s\n", WiFi.localIP().toString().c_str());
        bootLog("WiFi %s %d dBm", WiFi.localIP().toString().c_str(), (int)WiFi.RSSI());
        // This router's DHCP offers DNS 0.0.0.0 (option 6 unusable), so lwIP has no resolver: use public ones.
        if (WiFi.dnsIP(0) == IPAddress(0, 0, 0, 0)) {
          ip_addr_t d;
          IP_ADDR4(&d, 1, 1, 1, 1);
          dns_setserver(0, &d);
          IP_ADDR4(&d, 8, 8, 8, 8);
          dns_setserver(1, &d);
          Serial.println("[WIFI] DHCP gave no DNS, using 1.1.1.1 / 8.8.8.8");
          bootLog("DNS fallback 1.1.1.1 / 8.8.8.8");
        }
        st = St::Connected;
      } else if (now - stamp > kConnectTimeoutMs) {
        WiFi.disconnect();
        failures++;
        which ^= 1;
        stamp = now;
        // Round over (both SSIDs tried, or only one configured): backoff if we had a link before, else AP.
        if (retryFromAp || failures >= 2 || !(*c.wifi_ssid2 && *c.wifi_ssid)) {
          if (staFailures < 255) staFailures++;
          failures = 0;
          if (wasConnected && !retryFromAp && staFailures < 4) {
            st = St::Backoff;
          } else {
            if (!apUp) startAp();
            st = St::Ap;
          }
          retryFromAp = false;
        } else {
          beginSta(now);  // try the other SSID straight away
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
      if (now - stamp >= backoffMs(staFailures)) {
        cfgGet(c);
        failures = 0;
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
      // shortcut: AP_STA retry hops the radio channel, which can drop AP clients; skip while one is connected.
      if (now - stamp >= kApRetryMs) {
        if (WiFi.softAPgetStationNum() > 0) { stamp = now; break; }
        cfgGet(c);
        failures = 0;
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
