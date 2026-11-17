#include "dx_cluster.h"

#include <Arduino.h>
#include <WiFi.h>
#include <string.h>
#include <time.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "app/service_coords.h"
#include "backoff.h"
#include "dx_parse.h"
#include "geo.h"
#include "net/time_service.h"

namespace {
const uint32_t kLoginDelayMs = 1000, kCmdGapMs = 50, kKeepAliveMs = 30000, kSilenceMs = 5UL * 60 * 1000;
const int32_t kConnectTimeoutMs = 4000;  // blocks the net task while connecting (same as the original)

WiFiClient client;
bool up = false, haveAttempt = false, gotData = false, discard = false, loginSent = false;
uint8_t failures = 0;
uint32_t dropped = 0;
uint32_t lastAttempt, loginAt, lastRx, lastKeep, lastCmd;
char line[256];
size_t len;
char queue[320];  // pending config commands, '\n' separated
Config c;         // net task only
char curHost[64];
int32_t curPort;

void drop(bool failed) {
  client.stop();
  up = false;
  if (failed && failures < 255) failures++;
}

void queueCmd(const char* s) {
  if (strlen(queue) + strlen(s) + 2 < sizeof queue) { strcat(queue, s); strcat(queue, "\n"); }
}

void buildQueue() {
  queue[0] = 0;
  if (strlen(c.user_locator) >= 4) { char b[24]; snprintf(b, sizeof b, "set/qra %s", c.user_locator); queueCmd(b); }
  if (c.cluster_noann) queueCmd("set/noann");
  if (c.cluster_nowwv) queueCmd("set/nowwv");
  if (c.cluster_nowcy) queueCmd("set/nowcy");
  if (c.cluster_usefl && c.cluster_filters[0]) {
    // Several commands may be separated by newlines; blank lines are skipped.
    char f[sizeof c.cluster_filters];
    strlcpy(f, c.cluster_filters, sizeof f);
    for (char* s = strtok(f, "\r\n"); s; s = strtok(nullptr, "\r\n")) {
      while (*s == ' ') s++;
      if (*s) queueCmd(s);
    }
  } else {
    queueCmd("set/nofilter");
  }
}

void onLine() {
  line[len] = 0;
  if (!timeValid()) {  // spot dates need a valid clock
    if (!dropped++) Serial.println("[DX] clock not set: dropping spots");
    return;
  }
  Spot s;
  if (!parseDxLine(line, (uint32_t)time(nullptr), s)) return;
  double la, lo, ml, mo;
  if (s.loc[0] && locatorToLatLon(s.loc, la, lo)) {
    s.lat = (float)la; s.lon = (float)lo; s.hasLatLon = true;
    if (serviceCoords(ml, mo)) s.distKm = (int16_t)distanceKm(ml, mo, la, lo);
  }
  qrzOnNewSpot(s);
  g_store.dx.write([&](RingBuffer<Spot, kDxSpots>& r) { r.pushFront(s); });
}

void connectNow(uint32_t now) {
  haveAttempt = true;
  lastAttempt = now;
  strlcpy(curHost, c.cluster_host, sizeof curHost);
  curPort = c.cluster_port;
  if (!client.connect(curHost, (uint16_t)curPort, kConnectTimeoutMs)) { drop(true); return; }
  up = true; gotData = false; loginSent = false; discard = false;
  len = 0; queue[0] = 0;
  lastRx = lastKeep = lastCmd = now;
  loginAt = now + kLoginDelayMs;
}
}  // namespace

void dxTick(uint32_t now) {
  bool sta = false;
  g_store.net.read([&](const NetStatus& n) { sta = n.sta; });
  cfgGet(c);
  if (up && (!sta || strcmp(curHost, c.cluster_host) != 0 || curPort != c.cluster_port)) {
    drop(false);
    failures = 0;
  }
  if (!up) {
    if (!sta || !c.cluster_host[0]) return;
    if (haveAttempt && now - lastAttempt < backoffMs(failures)) return;
    connectNow(now);
    return;
  }
  if (!client.connected() && !client.available()) { drop(true); return; }

  for (int i = 0; i < 256 && client.available(); i++) {
    int ch = client.read();
    if (ch < 0) break;
    lastRx = now;
    if (!gotData) { gotData = true; failures = 0; }
    if (ch == '\n' || ch == '\r') {
      if (len && !discard) onLine();
      len = 0;
      discard = false;
    } else if (ch >= 32 && ch < 127 && !discard) {
      if (len < sizeof line - 1) line[len++] = (char)ch;
      else discard = true;  // over-long line: drop it up to the next newline
    }
  }

  if (now - lastRx > kSilenceMs) { drop(true); return; }
  if (!loginSent && (int32_t)(now - loginAt) >= 0) {
    client.print(c.user_callsign[0] ? c.user_callsign : "SWL");
    client.print("\r\n");
    loginSent = true;
    lastKeep = lastCmd = now;
    buildQueue();
  } else if (loginSent && queue[0] && now - lastCmd >= kCmdGapMs) {
    char* nl = strchr(queue, '\n');
    *nl = 0;
    client.print(queue);
    client.print("\r\n");
    memmove(queue, nl + 1, strlen(nl + 1) + 1);
    lastCmd = lastKeep = now;
  } else if (now - lastKeep > kKeepAliveMs) {
    client.print("\r\n");
    lastKeep = now;
  }
}
