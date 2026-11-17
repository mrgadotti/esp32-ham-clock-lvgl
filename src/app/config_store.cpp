#include "config_store.h"

#include <Preferences.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "board.h"
#include "config_codec.h"

#if __has_include("secrets.h")
#include "secrets.h"
#endif

// Build with -DCFG_LOG_HOLD to print how long the config mutex is held per update (target < 1 ms).
#ifdef CFG_LOG_HOLD
#include <Arduino.h>
#include <esp_timer.h>
#define HOLD_T0() int64_t holdT0 = esp_timer_get_time()
#define HOLD_LOG(what) Serial.printf("[CFG] %s held mutex %d us\n", what, (int)(esp_timer_get_time() - holdT0))
#else
#define HOLD_T0()
#define HOLD_LOG(what)
#endif

static Config g_cfg, g_cfgUi;
static SemaphoreHandle_t g_mutex;      // guards g_cfg; never held across NVS writes
static SemaphoreHandle_t g_saveMutex;  // serialises NVS saves (leaf lock; g_mutex may be taken inside it, not the reverse)
static std::atomic<uint32_t> g_ver{0};

void configStoreLoad(Config& c) {
  if (!g_mutex) g_mutex = xSemaphoreCreateMutex();
  if (!g_saveMutex) g_saveMutex = xSemaphoreCreateMutex();
  configDefaults(c);
  c.tft_inv = DISPLAY_INVERT_DEFAULT;
#ifdef SECRET_WIFI_SSID
  snprintf(c.wifi_ssid, sizeof c.wifi_ssid, "%s", SECRET_WIFI_SSID);
  snprintf(c.wifi_pass, sizeof c.wifi_pass, "%s", SECRET_WIFI_PASS);
#endif

  Preferences p;
  if (p.begin("hamclk", true)) {
    uint8_t* base = (uint8_t*)&c;
    for (size_t i = 0; i < kFieldCount; i++) {
      const ConfigField& f = kFields[i];
      uint8_t* d = base + f.offset;
      if (!p.isKey(f.nvsKey)) continue;
      switch (f.type) {
        case FT_STR: p.getString(f.nvsKey, (char*)d, f.size); break;
        case FT_INT: *(int32_t*)d = p.getInt(f.nvsKey); break;
        case FT_FLT: *(float*)d = p.getFloat(f.nvsKey); break;
        case FT_BOOL: *(bool*)d = p.getBool(f.nvsKey); break;
        case FT_LANG: *d = p.getUChar(f.nvsKey); break;
        case FT_BYTES:
        case FT_BLOB:
          if (p.getBytesLength(f.nvsKey) == f.size) p.getBytes(f.nvsKey, d, f.size);
          break;
      }
    }
    p.end();
  }
  configClamp(c);
  g_cfg = g_cfgUi = c;
  g_ver++;
}

bool configStoreSave(const Config& c) {
  Preferences p;
  if (!p.begin("hamclk", false)) return false;
  bool ok = true;
  const uint8_t* base = (const uint8_t*)&c;
  for (size_t i = 0; i < kFieldCount; i++) {
    const ConfigField& f = kFields[i];
    const uint8_t* s = base + f.offset;
    size_t n = 0, want = f.size;  // bytes the put must report
    switch (f.type) {
      case FT_STR: want = strlen((const char*)s); n = p.putString(f.nvsKey, (const char*)s); break;
      case FT_INT: n = p.putInt(f.nvsKey, *(const int32_t*)s); break;
      case FT_FLT: n = p.putFloat(f.nvsKey, *(const float*)s); break;
      case FT_BOOL: n = p.putBool(f.nvsKey, *(const bool*)s); break;
      case FT_LANG: n = p.putUChar(f.nvsKey, *s); break;
      case FT_BYTES:
      case FT_BLOB: n = p.putBytes(f.nvsKey, s, f.size); break;
    }
    if (f.type == FT_INT || f.type == FT_FLT) want = 4;
    else if (f.type == FT_BOOL || f.type == FT_LANG) want = 1;
    ok = ok && n == want;
  }
  p.end();
  return ok;
}

void cfgGet(Config& out) {
  xSemaphoreTake(g_mutex, portMAX_DELAY);
  out = g_cfg;
  xSemaphoreGive(g_mutex);
}

const Config& cfg() { return g_cfgUi; }

void cfgRefreshUiSnapshot() { cfgGet(g_cfgUi); }

uint32_t cfgVersion() { return g_ver; }

void cfgReadImpl(void (*fn)(const Config&, void*), void* ctx) {
  xSemaphoreTake(g_mutex, portMAX_DELAY);
  fn(g_cfg, ctx);
  xSemaphoreGive(g_mutex);
}

// Saves the latest published config, so concurrent updates cannot leave an older snapshot in NVS. The save buffer is
// static (guarded by g_saveMutex) to keep 1.3 KB off the caller's stack. If A's save fails and B's (later, newer)
// succeeds, A reports failure although its state did reach flash through B: harmless, the caller only warns.
static Config s_persist;
static bool persistLatest() {
  xSemaphoreTake(g_saveMutex, portMAX_DELAY);
  cfgGet(s_persist);
  bool ok = configStoreSave(s_persist);
  xSemaphoreGive(g_saveMutex);
  return ok;
}

bool cfgUpdateImpl(void (*fn)(Config&, void*), void* ctx) {
  HOLD_T0();
  xSemaphoreTake(g_mutex, portMAX_DELAY);
  fn(g_cfg, ctx);  // in place: readers wait on the mutex, so RAM state changes atomically
  configClamp(g_cfg);
  g_ver++;
  xSemaphoreGive(g_mutex);
  HOLD_LOG("cfgUpdate");
  bool ok = persistLatest();
  onConfigChanged();
  return ok;
}

// On an NVS save failure the new values stay applied in RAM, but out.ok is false (error "nvs_save").
bool cfgApplyJson(const char* json, ApplyResult& out) {
  ConfigPatch patch(json);  // parsed before the lock: only validate+commit run under it
  out = patch.parsed();
  if (!out.ok) return false;
  HOLD_T0();
  xSemaphoreTake(g_mutex, portMAX_DELAY);
  out = patch.apply(g_cfg);  // all-or-nothing: g_cfg is untouched on error
  if (out.ok) g_ver++;
  xSemaphoreGive(g_mutex);
  HOLD_LOG("cfgApplyJson");
  if (!out.ok) return false;
  if (!persistLatest()) {
    out.ok = false;
    snprintf(out.error, sizeof out.error, "nvs_save");
  }
  onConfigChanged();
  return out.ok;
}

uint8_t lastScreenLoad() {
  Preferences p;
  uint8_t v = 0;
  if (p.begin("hamclk", true)) { if (p.isKey("last_scr")) v = p.getUChar("last_scr"); p.end(); }
  return v;
}

void lastScreenSave(uint8_t id) {
  Preferences p;
  if (p.begin("hamclk", false)) { p.putUChar("last_scr", id); p.end(); }
}
