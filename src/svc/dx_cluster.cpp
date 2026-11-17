#include "dx_cluster.h"

#include <Arduino.h>
#include <WiFi.h>
#include <string.h>
#include <time.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "app/mem_log.h"
#include "app/service_coords.h"
#include "backoff.h"
#include "dx_parse.h"
#include "geo.h"
#include "net/time_service.h"
#include "svc/async_connect.h"
#include "svc/line_reader.h"
#include "svc/qrz.h"

namespace {
const uint32_t kLoginDelayMs = 1000, kCmdGapMs = 50, kKeepAliveMs = 30000, kSilenceMs = 5UL * 60 * 1000;

struct Cfg {
  char host[64], callsign[16], locator[11], filters[128];
  int32_t port;
  bool noann, nowwv, nowcy, usefl;
} c;  // net task only; refreshed when cfgVersion() moves
uint32_t cfgVer;
bool haveCfg;

AsyncConnect conn;
WiFiClient client;
bool up = false, connecting = false, haveAttempt = false, gotData = false, loginSent = false;
uint8_t failures = 0;
uint32_t dropped = 0;
uint32_t lastAttempt, loginAt, lastRx, lastKeep, lastCmd;
char line[256];
LineReader reader(line, sizeof line, true);
char queue[320];  // pending config commands, '\n' separated
char curHost[64];
int32_t curPort;

void refreshCfg() {
  uint32_t v = cfgVersion();
  if (haveCfg && v == cfgVer) return;
  haveCfg = true;
  cfgVer = v;
  cfgRead([](const Config& s) {
    strlcpy(c.host, s.cluster_host, sizeof c.host);
    strlcpy(c.callsign, s.user_callsign, sizeof c.callsign);
    strlcpy(c.locator, s.user_locator, sizeof c.locator);
    strlcpy(c.filters, s.cluster_filters, sizeof c.filters);
    c.port = s.cluster_port;
    c.noann = s.cluster_noann; c.nowwv = s.cluster_nowwv; c.nowcy = s.cluster_nowcy; c.usefl = s.cluster_usefl;
  });
}

void drop(bool failed) {
  conn.cancel();
  client.stop();
  up = connecting = false;
  if (failed) {
    if (failures < 255) failures++;
    lastAttempt = millis();  // backoff counts from the end of the failed attempt
  }
}

void queueCmd(const char* s) {
  if (strlen(queue) + strlen(s) + 2 < sizeof queue) { strcat(queue, s); strcat(queue, "\n"); }
}

void buildQueue() {
  queue[0] = 0;
  if (strlen(c.locator) >= 4) { char b[24]; snprintf(b, sizeof b, "set/qra %s", c.locator); queueCmd(b); }
  if (c.noann) queueCmd("set/noann");
  if (c.nowwv) queueCmd("set/nowwv");
  if (c.nowcy) queueCmd("set/nowcy");
  if (c.usefl && c.filters[0]) {
    // Several commands may be separated by newlines; blank lines are skipped.
    char f[sizeof c.filters];
    strlcpy(f, c.filters, sizeof f);
    for (char* s = strtok(f, "\r\n"); s; s = strtok(nullptr, "\r\n")) {
      while (*s == ' ') s++;
      if (*s) queueCmd(s);
    }
  } else {
    queueCmd("set/nofilter");
  }
}

void onLine(const char* l) {
  if (!timeValid()) {  // spot dates need a valid clock
    if (!dropped++) Serial.println("[DX] clock not set: dropping spots");
    return;
  }
  Spot s;
  if (!parseDxLine(l, (uint32_t)time(nullptr), s)) return;
  double la, lo, ml, mo;
  if (s.loc[0] && locatorToLatLon(s.loc, la, lo) && serviceCoords(ml, mo)) s.distKm = (int16_t)distanceKm(ml, mo, la, lo);
  qrzOnNewSpot(s);
  g_store.dx.write([&](RingBuffer<Spot, kDxSpots>& r) { r.pushFront(s); });
}

void connectNow(uint32_t now) {
  haveAttempt = true;
  lastAttempt = now;
  strlcpy(curHost, c.host, sizeof curHost);
  curPort = c.port;
  MEM_DIAG_ONCE("dx: before connect");
  conn.start(curHost, (uint16_t)curPort, now);
  connecting = true;
}

void onConnected(uint32_t now) {
  MEM_DIAG_ONCE("dx: after connect");
  up = true; gotData = false; loginSent = false;
  reader.reset();
  queue[0] = 0;
  lastRx = lastKeep = lastCmd = now;
  loginAt = now + kLoginDelayMs;
}
}  // namespace

void dxTick(uint32_t now) {
  bool sta = false;
  g_store.net.read([&](const NetStatus& n) { sta = n.sta; });
  refreshCfg();
  if ((up || connecting) && (!sta || strcmp(curHost, c.host) != 0 || curPort != c.port)) {
    drop(false);
    failures = 0;
  }
  if (connecting) {
    AsyncConnect::State s = conn.poll(now, client);
    if (s == AsyncConnect::Dns || s == AsyncConnect::Connecting) return;
    connecting = false;
    if (s == AsyncConnect::Done) onConnected(now);
    else drop(true);
    return;
  }
  if (!up) {
    if (!sta || !c.host[0]) return;
    if (haveAttempt && now - lastAttempt < backoffMs(failures)) return;
    connectNow(now);
    return;
  }
  if (!client.connected() && !client.available()) { drop(true); return; }

  if (reader.pump(client, 256, onLine)) {
    lastRx = now;
    if (!gotData) { gotData = true; failures = 0; }
  }

  if (now - lastRx > kSilenceMs) { drop(true); return; }
  if (!loginSent && (int32_t)(now - loginAt) >= 0) {
    client.print(c.callsign[0] ? c.callsign : "SWL");
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
