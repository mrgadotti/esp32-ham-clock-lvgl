#include "config_store.h"

#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "board.h"
#include "config_codec.h"

#if __has_include("secrets.h")
#include "secrets.h"
#endif

static Config g_cfg, g_cfgUi;
static SemaphoreHandle_t g_mutex;

__attribute__((weak)) void onConfigChanged() {}

void configStoreLoad(Config& c) {
  if (!g_mutex) g_mutex = xSemaphoreCreateMutex();
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

bool cfgUpdate(void (*fn)(Config&)) {
  xSemaphoreTake(g_mutex, portMAX_DELAY);
  Config c = g_cfg;
  fn(c);
  configClamp(c);
  g_cfg = c;
  bool ok = configStoreSave(c);
  xSemaphoreGive(g_mutex);
  onConfigChanged();
  return ok;
}

bool cfgApplyJson(const char* json, ApplyResult& out) {
  xSemaphoreTake(g_mutex, portMAX_DELAY);
  Config c = g_cfg;
  out = configApplyJson(c, json);
  bool ok = out.ok;
  if (ok) {
    g_cfg = c;
    ok = configStoreSave(c);
    if (!ok) snprintf(out.error, sizeof out.error, "nvs_save");
  }
  xSemaphoreGive(g_mutex);
  if (out.ok) onConfigChanged();
  return ok;
}
