# WP5 report: config store and callers

Findings: C-06, M-02 (additive API), K-02/A-03 (API), N-04, C-07 (core), C-14 (data_store.h part).

## What changed
- `cfgUpdate` mutates the live config in place under the mutex (fn + clamp + version bump only), releases it, then saves the latest published config under a separate `g_saveMutex`. No Config copy on the update path; concurrent updates cannot leave a stale snapshot in NVS. Build with `-DCFG_LOG_HOLD` to print the mutex hold time per update/apply (`[CFG] cfgUpdate held mutex N us`).
- `cfgApplyJson` applies in place on `g_cfg` (no stack Config) and also saves outside the mutex.
- New additive API in `config_store.h`: `cfgVersion()`, `cfgRead(f)` (narrow getter: copy a few fields under the mutex), `template cfgUpdate(F f)` accepting capturing lambdas (the `pend*` globals in the screens can be deleted; `cfgUpdate(void(*)(Config&))` callers still compile). `cfg()`/`cfgGet()` unchanged.
- `configApplyJson` is in place: pass 0 validates every field into a 128 B scratch, pass 1 commits; no second `Config` (-1.3 KB stack). All-or-nothing preserved.
- `serviceCoords` caches lat/lon/locator result per `cfgVersion()`; no 1.3 KB copy per call.
- `Guarded::ver_` is `std::atomic<uint32_t>`.

## Numbers (cyd)
Flash 1786889 -> 1787349 B (+460), static RAM 118260 -> 118316 B (+56). esp32-38pin: 1786825 B / 118292 B. Static RAM does not drop here: the other static Config copies live in files owned by other WPs (weather, wifi_manager, dx_cluster, uart2) and the config_store keeps `g_cfgUi` for `cfg()`.

## Tests
`pio test -e native` green. New: `test/test_config_store` (runs the real config_store.cpp against stub Preferences/FreeRTOS: version bumps, clamp, mutex free during every NVS put, RAM change kept on save failure, version unchanged on rejected JSON), `test_in_place_apply` in test_config_codec.

## Not done / for other WPs
- C-14 `volatile` in src/svc/uart2.cpp (WP6b file) left as is.
- Callers still to migrate: services (`cfgGet` -> `cfgVersion` + `cfgRead`) in WP6a/6b, `pend*` globals in screens (WP3/WP4), settings_screen.

## Hardware checks still needed
- `[CFG]` hold time < 1 ms with `-DCFG_LOG_HOLD` on Settings slider release.
- Net-task stack watermark >= 3.5 KB free after a web Settings save (expected +1.3 KB from the in-place apply; further gain needs the service migrations).

## Follow-ups (review)
- `persistLatest` now uses a `static Config s_persist` guarded by `g_saveMutex` instead of 1.3 KB on the caller's stack. Static RAM cyd: 118316 -> 119652 B (+1336); esp32-38pin 119628 B. Flash cyd 1787641 B, esp32-38pin 1787117 B.
- `cfgApplyJson` parses before taking `g_mutex`: new `ConfigPatch` class in config_codec (constructor parses, `apply()` validates and commits); `configApplyJson(Config&, json)` is kept as a wrapper. The class is opaque (pimpl) because including ArduinoJson.h in the header pulls `<cmath>` and breaks boot_screen.cpp (`static lv_obj_t* log`).
- `serviceCoords` zero-initialises its local `Fixed f{}`.
- `kMaxFieldSize` moved to config_codec.h; `test_json_fields_fit_scratch` asserts every field with a jsonKey fits it.
- Comment in `persistLatest` documents "A failed, B succeeded": A reports failure although its state reached flash through B.
- Limitation: the serialisation done by `g_saveMutex` cannot be tested with the single-threaded stub (test_config_store only proves the config mutex is free during puts); it needs a hardware or multi-threaded check.
