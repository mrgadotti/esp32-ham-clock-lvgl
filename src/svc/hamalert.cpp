#include "hamalert.h"

#include <Arduino.h>
#include <WiFi.h>
#include <string.h>
#include <time.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "app/mem_log.h"
#include "app/service_coords.h"
#include "geo.h"
#include "hamalert_parse.h"
#include "net/time_service.h"
#include "svc/async_connect.h"
#include "svc/line_reader.h"
#include "svc/qrz.h"
#include "svc/spot_scratch.h"

// Poll as a state machine, one small step per net tick: connect -> swallow banner -> login/commands -> read lines.
// Each state has its own timeout and no state waits inside a tick.
namespace {
constexpr uint32_t kPeriodMs = 60000, kTotalMs = 9000, kIdleMs = 1800, kBannerMs = 1200;
constexpr size_t kMax = kSpotScratchN, kLineMax = 1400, kReadBudget = 1024;
constexpr uint16_t kGapMs[] = {0, 180, 220, 160};  // before: user, pass, set/json, sh/dx 30
enum class St : uint8_t { Idle, Connecting, Banner, Login, Read };

struct Cfg { char host[64], user[32], pass[32]; int32_t port; } cc;
uint32_t cfgVer;
bool haveCfg;

AsyncConnect conn;
WiFiClient client;
char lineBuf[kLineMax + 1];
LineReader reader(lineBuf, sizeof lineBuf, true);
St st = St::Idle;
uint32_t last, tStart, tStep, lastRx, nowUtc;
bool tried;
size_t n;
uint8_t step;

void refreshCfg() {
  uint32_t v = cfgVersion();
  if (haveCfg && v == cfgVer) return;
  haveCfg = true;
  cfgVer = v;
  cfgRead([](const Config& c) {
    strlcpy(cc.host, c.hama_host, sizeof cc.host);
    strlcpy(cc.user, c.hama_user, sizeof cc.user);
    strlcpy(cc.pass, c.hama_pass, sizeof cc.pass);
    cc.port = c.hama_port;
  });
}

void abortPoll() {
  conn.cancel();
  client.stop();
  reader.reset();
  spotScratchHeld() = false;
  st = St::Idle;
}

void finish() {
  Spot* spots = spotScratch();
  client.stop();
  spotScratchHeld() = false;
  st = St::Idle;
  if (n == 0) {
    Serial.println("[HAMALERT] no spots, keeping old list");
    return;
  }
  double ml, mo;
  bool haveMe = serviceCoords(ml, mo);
  for (size_t i = 0; i < n; i++) {
    double la, lo;
    if (haveMe && spots[i].loc[0] && locatorToLatLon(spots[i].loc, la, lo)) spots[i].distKm = (int16_t)distanceKm(ml, mo, la, lo);
    qrzOnNewSpot(spots[i]);
  }
  g_store.hamalert.write([&](RingBuffer<Spot, 30>& r) {
    r.clear();
    for (size_t i = n; i-- > 0;) r.pushFront(spots[i]);  // server sends newest first
  });
  Serial.printf("[HAMALERT] %u spots\n", (unsigned)n);
}

void onLine(const char* line) {
  if (n < kMax && hamalertParseLine(line, nowUtc, spotScratch()[n])) n++;
}
}  // namespace

void hamalertTick(uint32_t now) {
  bool sta = false;
  g_store.net.read([&](const NetStatus& s) { sta = s.sta; });
  refreshCfg();
  bool enabled = sta && cc.host[0] && cc.port > 0 && cc.user[0] && cc.pass[0] && timeValid();
  if (st == St::Idle) {
    if (!enabled || (tried && now - last < kPeriodMs)) return;
    tried = true;
    last = now;
    n = 0;
    step = 0;
    nowUtc = (uint32_t)time(nullptr);
    spotScratchHeld() = true;
    reader.reset();
    MEM_DIAG_ONCE("hamalert: before connect");
    conn.start(cc.host, (uint16_t)cc.port, now);
    st = St::Connecting;
    return;
  }
  if (!enabled) { abortPoll(); return; }

  switch (st) {
    case St::Connecting: {
      AsyncConnect::State s = conn.poll(now, client);
      if (s == AsyncConnect::Dns || s == AsyncConnect::Connecting) return;
      if (s != AsyncConnect::Done) {
        Serial.println("[HAMALERT] connect failed, keeping old list");
        abortPoll();
        return;
      }
      MEM_DIAG_ONCE("hamalert: after connect");
      tStart = tStep = now;
      st = St::Banner;
      return;
    }
    case St::Banner:  // swallow banner/prompt
      for (int i = 0; i < 512 && client.available(); i++) client.read();
      if (!client.connected()) { abortPoll(); return; }
      if (now - tStart >= kBannerMs) st = St::Login;
      return;
    case St::Login:
      if (now - tStep < kGapMs[step]) return;
      if (step == 0) { client.print(cc.user); client.print("\n"); }
      else if (step == 1) { client.print(cc.pass); client.print("\n"); }  // never logged
      else client.print(step == 2 ? "set/json\n" : "sh/dx 30\n");
      tStep = now;
      if (++step < 4) return;
      tStart = lastRx = now;
      st = St::Read;
      return;
    case St::Read:
      if (reader.pump(client, kReadBudget, onLine)) lastRx = now;
      if (n >= kMax || now - tStart >= kTotalMs || (n > 0 && now - lastRx > kIdleMs) ||
          (!client.connected() && !client.available())) {
        finish();
      }
      return;
    default:
      return;
  }
}
