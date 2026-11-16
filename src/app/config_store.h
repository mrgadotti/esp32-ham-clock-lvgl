#pragma once
#include "config.h"
#include "config_codec.h"

// Persistence in NVS namespace "hamclk". Call configStoreLoad() once in setup() before any other call.
void configStoreLoad(Config&);        // defaults (+ secrets.h WiFi), overlaid with whatever NVS holds; also becomes the live config
bool configStoreSave(const Config&);  // false if any NVS write failed

void cfgGet(Config& out);             // any task: copy taken under the mutex
// UI task only: snapshot refreshed by cfgRefreshUiSnapshot(), so it never races with cfgUpdate from other tasks.
const Config& cfg();
void cfgRefreshUiSnapshot();          // UI task: call at boot and when handling ConfigChanged
// fn runs under a non-recursive mutex: it must not call cfgGet/cfgUpdate/cfgApplyJson.
// Mutate a copy, clamp, publish, save to NVS, then call onConfigChanged(). Returns false if the save failed.
bool cfgUpdate(void (*fn)(Config&));

// Applies a JSON patch under the mutex; on success saves and fires onConfigChanged().
bool cfgApplyJson(const char* json, ApplyResult& out);

// Weak no-op here; the UI layer overrides it to post EventKind::ConfigChanged.
void onConfigChanged();
