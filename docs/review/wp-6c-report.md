# WP6c report: net web, WiFi, time

Findings: C-04, C-05 (decision), C-07 (web part), M-05 (wifi publish), N-07, L-01, C-12 (listed), L-02 (skipped), A-05 leftovers.

## What changed
- C-04: every POST (`/api/save`, `/api/reset_touch`, `/api/bridge_bt_scan`, `/api/bridge_bt_pair`) goes through `postAllowed()`: `Content-Type` must start with `application/json` (else 415), and if an `Origin` header is present it must equal `http://<Host>` (else 403). `server.collectHeaders({"Content-Type","Origin"})` in `webInit`. A cross-site page cannot send JSON without a preflight and the device does not answer OPTIONS.
  Test: `curl -i -X POST -H 'Content-Type: text/plain' -d '{"tft_backlight":50}' http://<ip>/api/save` -> 415; same with `-H 'Content-Type: application/json'` -> 200 `{"status":"ok",...}`; `-H 'Origin: http://evil.example'` with JSON -> 403.
- web/config.html: `post()` helper sends `Content-Type: application/json` on all 4 POSTs (scan and reset_touch send `{}`); the page's own save/scan/pair/recalibrate go through it. Dead PT entry "Touch rotation" removed (the option itself was already gone from the field table). `uart2_mode` is no longer derived/sent (nothing in firmware reads it; the key stays in config_codec for WP7's A-05 removal). Saving an AP password of 1-7 chars is rejected client-side (it used to silently start an open AP). EN/PT tables checked in sync.
- N-07: replies use `server.send_P(code, type, buf, strlen)`: the core's `send(code,type,const char*)` copies the body into a `String` (up to 6 KB) first. handleConfig already built the JSON once into `httpBody`. No `String` is built per reply; the only remaining ones are the small header/Origin strings per POST and `server.arg("plain")` (WebServer API returns a copy).
- C-07 web part: handleSave already uses `cfgApplyJson` (ConfigPatch, in place, no stack Config); nothing else to change there. `localTimeOf` now uses `cfgRead` for the timezone instead of a 1.3 KB `cfgGet` copy on the caller's stack.
- M-05: `wifi_manager.cpp publish()` no longer calls `toString()`/`WiFi.SSID()`: IP via `snprintf` from the raw address, SSID via `esp_wifi_get_config`, both rebuilt only when sta/ap/address changes. Per-second cost is only `WiFi.status()`, `RSSI()`, the store write.
- L-01: `src/svc/uart2.cpp` (WP6b-owned file, hunk at the top only): local Hinnant copy deleted, `syncClock` uses `gpsToUnix` from `lib/core/gps_time.h` (WP8, tested). Same math as before, -12 lines.

## Decisions
- C-05 (TLS `setInsecure()` + secrets in the URL): documented, not implemented. Pinning roots costs +2..8 KB flash, needs valid time before the first fetch and adds TLS heap/handshake risk next to the M-06 fragmentation work. Suggested README text (WP9 owns README): "HTTPS requests do not verify the server certificate; QRZ and OpenWeather credentials can be captured by an active attacker on the same network. Use a trusted WiFi."
- AP: kept the documented default password `1234567890` and the open AP when `ap_pass` is shorter than 8 chars (logged on serial). Recommendation for README: change the AP password on first use; the AP serves the unauthenticated config page. The web page now blocks accidental 1-7 char passwords.
- L-02 (TZ string + DST): skipped, needs a config key plus edits in clock_screen.cpp and wx_common.cpp (other WPs).
- C-12: for the integrator (aprs_beacon.cpp is WP6b's): change `kProjectComment` at `src/svc/aprs_beacon.cpp:16` to `https://github.com/mrgadotti/esp32-ham-clock-lvgl` (origin remote) or drop the 5th-beacon advert. Not touched.

## Numbers (cyd)
Flash 1737365 -> 1737913 B (+548), static RAM 116476 -> 116556 B (+80: publish() statics). esp32-38pin builds: 1737389 B / 116532 B. `config_html.h` gzip 5459 -> 5481 B. `pio test -e native` 170/170. `tools/check_text_fit.py`: 2 pre-existing overflows (NIGHT col, long date), unrelated to this WP.

## Hardware checks still needed
- curl matrix above against the device (415/403/200), plus the page itself: Save, Scan, Pair, Recalibrate touch, Export/Import.
- Net task stack watermark after a Settings save from the page; heap churn (no per-second allocs) in `[MEM]` over a few minutes.
- Status line in the page and on-device WiFi screen still show IP/SSID in STA and AP-fallback modes.
