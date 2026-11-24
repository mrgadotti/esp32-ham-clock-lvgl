#ifdef SCREEN_TEST
#include "screen_test.h"

#include <Arduino.h>
#include <stdlib.h>

#include "app/config_store.h"
#include "app/data_store.h"
#include "app/mem_log.h"
#include "i18n.h"
#include "svc/aprs_ingest.h"
#include "ui/ui_manager.h"

static volatile bool capturing;
static struct { bool active; int16_t x1, y1, x2, y2; uint32_t t0, dur; } tch;

static constexpr uint32_t HOLD_MS = 120;  // press time after the move ends, long enough for two indev reads

void screenTestInit() { Serial.println("[ST] ready"); }

bool screenTestCapturing() { return capturing; }

// Frame: magic(4) x1 y1 x2 y2 (u16 LE) RGB565 LE bytes, u16 byte-sum. Text "[ST] shot_end" closes a frame.
void screenTestFlush(const lv_area_t* a, const uint8_t* px, bool last) {
  static const uint8_t magic[4] = {0xA5, 0x5A, 0xC3, 0x3C};
  uint16_t hdr[4] = {(uint16_t)a->x1, (uint16_t)a->y1, (uint16_t)a->x2, (uint16_t)a->y2};
  size_t n = (size_t)(a->x2 - a->x1 + 1) * (a->y2 - a->y1 + 1) * 2;
  uint16_t sum = 0;
  for (size_t i = 0; i < n; i++) sum += px[i];
  Serial.write(magic, 4);
  Serial.write((const uint8_t*)hdr, 8);
  Serial.write(px, n);
  Serial.write((const uint8_t*)&sum, 2);
  if (last) {
    capturing = false;
    Serial.print("\n[ST] shot_end\n");
  }
}

bool screenTestTouch(int16_t& x, int16_t& y) {
  if (!tch.active) return false;
  uint32_t t = millis() - tch.t0;
  if (t >= tch.dur + HOLD_MS) { tch.active = false; return false; }
  uint32_t k = t < tch.dur ? t : tch.dur;
  float f = tch.dur ? (float)k / tch.dur : 1.0f;
  x = tch.x1 + (int)((tch.x2 - tch.x1) * f);
  y = tch.y1 + (int)((tch.y2 - tch.y1) * f);
  return true;
}

static void touchStart(int x1, int y1, int x2, int y2, uint32_t dur) {
  tch = {true, (int16_t)x1, (int16_t)y1, (int16_t)x2, (int16_t)y2, millis(), dur};
  lv_display_trigger_activity(nullptr);
}

// Runtime-only change of the UI snapshot: never saved to NVS, reverted by the next ConfigChanged.
static Config& uiCfg() { return const_cast<Config&>(cfg()); }

// "fill N": N fake rows in every list store (screenshots of the spot / APRS tables without network).
static void fill(int n) {
  for (int i = n; i > 0; i--) {
    Spot sp = {};
    sp.utc = 1700000000u + i * 137;
    sp.khz = 14000 + i * 13;
    snprintf(sp.call, sizeof sp.call, "PY%dABC", i % 10);
    snprintf(sp.comment, sizeof sp.comment, "msg %d", i);
    snprintf(sp.country, sizeof sp.country, "Brazil");
    sp.band = 4;
    sp.mode = 1 + i % 3;
    g_store.dx.write([&](auto& r) { r.pushFront(sp); });
    g_store.pota.write([&](auto& r) { r.pushFront(sp); });
    g_store.hamalert.write([&](auto& r) { r.pushFront(sp); });
    AprsStation a = {};
    a.utc = sp.utc;
    snprintf(a.call, sizeof a.call, "PU2MM-%d", i % 10);
    a.sym[0] = '/';
    a.sym[1] = '>';
    a.freqMhz = 144.8f;
    a.distKm = (int16_t)(i * 7);
    a.source = 'I';
    g_store.aprs.write([&](auto& r) { r.pushFront(a); });
  }
}

// "msg <FROM> <text...> [#<n>]": a TNC2 message addressed to us, as if it came from APRS-IS.
static void fakeMessage(char* from, char* text) {
  static int counter;
  int no = ++counter;
  char* t = text ? text : (char*)"";
  char* h = strrchr(t, '#');
  if (h && h > t && h[-1] == ' ' && h[1]) { no = atoi(h + 1); h[-1] = 0; }
  char me[24], out[160];
  if (cfg().aprs_ssid > 0) snprintf(me, sizeof me, "%s-%d", cfg().aprs_callsign, (int)cfg().aprs_ssid);
  else snprintf(me, sizeof me, "%s", cfg().aprs_callsign);
  snprintf(out, sizeof out, "%s>APRS::%-9s:%s{%d", from, me, t, no);
  aprsIngestLine(out, 'I');
}

static void run(char* line) {
  char* cmd = strtok(line, " \t\r");
  if (!cmd) return;
  if (!strcmp(cmd, "msg")) {
    char* from = strtok(nullptr, " \t\r");
    if (from) fakeMessage(from, strtok(nullptr, "\r"));
    Serial.printf("[ST] ok %s\n", cmd);
    return;
  }
  char* arg = strtok(nullptr, " \t\r");
  int v[5] = {0}, n = 0;
  for (char* p = arg; p && n < 5; p = strtok(nullptr, " \t\r")) v[n++] = atoi(p);
  if (!strcmp(cmd, "goto")) {
    uiRequest((ScreenId)v[0]);
  } else if (!strcmp(cmd, "shot")) {
    capturing = true;
    lv_obj_invalidate(lv_screen_active());
    return;  // "[ST] shot_end" is the ack
  } else if (!strcmp(cmd, "fill")) {
    fill(v[0]);
  } else if (!strcmp(cmd, "tap")) {
    touchStart(v[0], v[1], v[0], v[1], 0);
  } else if (!strcmp(cmd, "swipe")) {  // x1 y1 x2 y2 [ms]
    touchStart(v[0], v[1], v[2], v[3], n > 4 && v[4] > 0 ? v[4] : 300);
  } else if (!strcmp(cmd, "lang")) {
    Serial.printf("[ST] lang was=%d\n", (int)uiCfg().tft_lang);
    uiCfg().tft_lang = (arg && (!strcmp(arg, "pt") || !strcmp(arg, "1"))) ? 1 : 0;
    i18nSetLang((Lang)uiCfg().tft_lang);
  } else if (!strcmp(cmd, "autosw")) {
    uiCfg().tft_autosw = v[0] != 0;
  } else if (!strcmp(cmd, "mem")) {
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    Serial.printf("[ST] mem lvfree=%u free=%u largest=%u minfree=%u\n", (unsigned)mon.free_size, (unsigned)ESP.getFreeHeap(),
                  (unsigned)memLargest8(), (unsigned)ESP.getMinFreeHeap());
    return;
  } else if (strcmp(cmd, "ping")) {
    Serial.printf("[ST] err unknown '%s'\n", cmd);
    return;
  }
  Serial.printf("[ST] ok %s\n", cmd);
}

void screenTestPoll() {
  static char buf[128];
  static uint8_t len;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      buf[len] = 0;
      len = 0;
      run(buf);
    } else if (len < sizeof buf - 1) {
      buf[len++] = c;
    }
  }
}
#endif
