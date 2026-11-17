#pragma once
#include <stdint.h>

#include "config.h"
#include "config_codec.h"

// Persistence in NVS namespace "hamclk". Call configStoreLoad() once in setup() before any other call.
void configStoreLoad(Config&);        // defaults (+ secrets.h WiFi), overlaid with whatever NVS holds; also becomes the live config
bool configStoreSave(const Config&);  // false if any NVS write failed

void cfgGet(Config& out);             // any task: copy taken under the mutex (1.3 KB: prefer cfgRead for a few fields)
// UI task only: snapshot refreshed by cfgRefreshUiSnapshot(), so it never races with cfgUpdate from other tasks.
const Config& cfg();
void cfgRefreshUiSnapshot();          // UI task: call at boot and when handling ConfigChanged

// Last manually selected ScreenId: its own NVS key (not in Config, so web JSON/backup are unaffected). 0 = never saved.
uint8_t lastScreenLoad();
void lastScreenSave(uint8_t id);

// Bumps on every published change (cfgUpdate, cfgApplyJson). Services poll it and refresh their cached fields only
// when it moves, instead of copying the whole Config every tick.
uint32_t cfgVersion();

// Any task: runs f(const Config&) under the mutex. Copy the few fields you need and return; no cfg* calls inside.
template <class F> void cfgRead(F f);

// Runs f(Config&) on the live config under the mutex, clamps, publishes, releases the mutex, then saves to NVS and
// calls onConfigChanged(). f must be short and must not call cfg*. Returns false if the save failed (the change stays
// applied in RAM). f may be a capturing lambda: no file-scope globals needed to pass values in.
template <class F> bool cfgUpdate(F f);

// Applies a JSON patch in place (all-or-nothing); saves and fires onConfigChanged(). If the NVS save fails, out.ok is
// false ("nvs_save") but the change stays applied in RAM. Callers must check out.ok.
bool cfgApplyJson(const char* json, ApplyResult& out);

// Weak no-op here; the UI layer overrides it to post EventKind::ConfigChanged.
void onConfigChanged();

// Non-template cores (config_store.cpp).
void cfgReadImpl(void (*fn)(const Config&, void*), void* ctx);
bool cfgUpdateImpl(void (*fn)(Config&, void*), void* ctx);

template <class F> void cfgRead(F f) {
  cfgReadImpl([](const Config& c, void* p) { (*static_cast<F*>(p))(c); }, &f);
}
template <class F> bool cfgUpdate(F f) {
  return cfgUpdateImpl([](Config& c, void* p) { (*static_cast<F*>(p))(c); }, &f);
}
