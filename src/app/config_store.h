#pragma once
#include "config.h"

// Persistence in NVS namespace "hamclk". Call configStoreLoad() once in setup() before any other call.
void configStoreLoad(Config&);        // defaults (+ secrets.h WiFi), overlaid with whatever NVS holds; also becomes the live config
bool configStoreSave(const Config&);  // false if any NVS write failed

void cfgGet(Config& out);             // any task: copy taken under the mutex
const Config& cfg();                  // UI task only: unsynchronized reference
// Mutate a copy, clamp, publish, save to NVS, then call onConfigChanged(). Returns false if the save failed.
bool cfgUpdate(void (*fn)(Config&));

// Weak no-op here; the UI layer overrides it to post EventKind::ConfigChanged.
void onConfigChanged();
