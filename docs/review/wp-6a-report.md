# WP6a report: Net HTTP, JSON, pollers

Changes
- http_fetch: `fetch` is non-template (F-11, one HTTPClient/TLS setup); both JSON paths and httpGet read through `Stream&`, so `Reader<WiFiClient>` is gone (F-09). C-16 needs no change (bounded, documented).
- web_config (F-09 web part, N-07): no `String` serialization or `deserializeJson(String)`; JSON replies go through `serializeJson(d, char*)` into the shared buffer, `/api/config` no longer mallocs 6 KB. `Reader<String>` and `Writer<String>` are gone from `nm`.
- M-01: one shared `httpBody[6144]` (http_fetch.h) replaces propagation's 8 KB and QRZ's 4 KB buffers; also the web scratch. All users run in the net task.
- M-05: pota uses `spotScratch()` (src/svc/spot_scratch.h, 30 Spot, static) instead of malloc/free per poll; HamAlert (WP6b) can use the same helper. POTA reads its URL via `cfgRead` instead of a 1.3 KB `Config` on the stack.
- C-03: QRZ `enabled()` and `login()` use `cfgRead` (consistent user/pass pair under the config mutex); no `cfg()` use left outside the UI task in src/svc/qrz.cpp. Test: `test_credentials_pair_consistent_during_save` (test_config_store).
- F-10: weather_json parses through `JsonVariantConst` locals (hamalert_parse has no proxy chains, unchanged).
- WP7 carry-over: `copyText` drops U+00A0..U+00BF except U+00B0 and U+00F7 (test_text_drops_glyphs_missing_from_fonts).

Numbers (cyd): flash 1,775,201 -> 1,738,805 B (-36.4 KB); static RAM 119,620 -> 117,076 B. esp32-38pin: flash 1,738,285, RAM 117,052.
`nm -S -C`: Reader<String>, Writer<String>, Reader<WiFiClient> 0 occurrences (before: 2, 2, 9).

Notes
- tools/check_text_fit.py reports 5 overflows in the APRS list: identical on main, not related to this WP.
- Hardware still needed: one POTA/propagation/QRZ/weather fetch each, `/api/config`, `/api/status`, `/api/bridge_bt_status` replies, Settings save of QRZ credentials while a lookup runs.
