# Library audit: hand-written code vs ready-made libraries

Scope: Arduino-ESP32 2.0.17 core, PlatformIO registry, LVGL 9.6 / ArduinoJson 7.4.3 already in use.
Result: no replacement made. The code that is not already a thin wrapper over a core API is either
domain-specific, must stay pure C++ for the native test env (`lib/core`, no Arduino.h), or a library
would cost more flash/RAM than the 5 to 60 lines it replaces. Flash/RAM delta is 0 B for every row
because no code changed.

| Candidate | Hand-written code (file) | Library considered | Decision | Reason | Flash/RAM delta |
|---|---|---|---|---|---|
| HTTP/TLS client | `src/net/http_fetch.cpp` | Core `HTTPClient` + `WiFiClientSecure` | Already uses core | File only wraps HTTPClient (body read loop with overflow check, TLS guard, ArduinoJson streaming). Nothing to swap. | 0 |
| Non-blocking connect | `src/svc/async_connect.cpp` | Core `WiFiClient::connect` (blocking, up to 4 s) | Kept | Core has no async connect; lwIP `dns_gethostbyname` + non-blocking `connect`/`select` is the minimum. AsyncTCP is a callback/task design with extra RAM and a different threading model. | 0 |
| WiFi reconnect / AP fallback | `src/net/wifi_manager.cpp` | `WiFiMulti`, WiFi events | Kept | `WiFiMulti` blocks in `run()`, scans every attempt, picks by RSSI and has no backoff or AP-fallback state; events would add callbacks on the WiFi task and change the tested backoff timeline. Already uses core `WiFi.*` for everything else. | 0 |
| NTP | `src/net/time_service.cpp` | `configTime` / `esp_sntp` | Already uses core | 33 lines: `configTime`, `sntp_set_time_sync_notification_cb`, hourly re-sync. | 0 |
| Web server | `src/net/web_config.cpp` | Core `WebServer`, `ESPAsyncWebServer`, `ESPmDNS` | Already uses `WebServer` | ESPAsyncWebServer needs AsyncTCP (extra task and heap, GPL-compatible but unpinned/forked). mDNS is not used and the Host check intentionally rejects `.local`. CSRF guard stays as is. | 0 |
| URL percent-encode | `lib/core/url_encode.cpp` | `HTTPClient::escapeString`, `WebServer::urlDecode` | Kept | Core versions return Arduino `String` (heap churn on the net task) and are not available natively; the 12-line encoder is unit tested and truncates on escape boundaries. | 0 |
| NMEA parsing | `lib/core/nmea.cpp` | TinyGPSPlus (LGPL-2.1) | Kept | TinyGPSPlus includes Arduino.h (breaks native tests) and keeps per-field state objects (more RAM). The parser also handles the `$GPSESP32[...]` frame and per-constellation GSV counts, which TinyGPSPlus lacks. | 0 |
| Config JSON codec | `lib/core/config_codec.cpp` | ArduinoJson features | Already uses ArduinoJson | Table-driven field list (`kFields`) shared by NVS and JSON; no hand-rolled JSON. | 0 |
| NVS storage | `src/app/config_store.cpp` | `Preferences` | Already uses core | Same key names kept. | 0 |
| Ring buffer | `lib/core/ring_buffer.h` | `etl::circular_buffer`, `CircularBuffer` libs | Kept | 33-line header, no heap, newest-first plus `moveToFrontIf`, which no library offers. Adding a dependency would cost more than it saves. | 0 |
| Backoff | `lib/core/backoff.cpp` | none (exponential-backoff libs are Arduino-only) | Kept | 4 lines. | 0 |
| Base64 / CRC / hash | none found | `mbedtls` | n/a | The repo has no hand-written base64, CRC or hash; NMEA XOR checksum is a 4-line loop inside the parser. | 0 |
| Maidenhead / distance / bearing | `lib/core/geo.cpp` | Maidenhead / geographic libs on the registry | Kept | Registry libraries are Arduino-flavoured, unmaintained or larger than the 66 lines here, and the radius (6366.71 km) and rounding must match the original clock behaviour; covered by native tests. | 0 |
| KISS / AX.25 framing | `lib/core/ax25_kiss.cpp` | Arduino APRS/AX.25 libs (LibAPRS, etc.) | Kept | Those bundle modem/hardware code and Arduino.h; only encode/deframe/TNC2 text is needed and it is unit tested. | 0 |
| Touch debounce | `src/hal/touch.cpp`, `touch_cal_ui.cpp` | `XPT2046_Touchscreen` (already used) | Kept | Two-miss pressure debounce is app policy, one line each. | 0 |
| PWM backlight | `src/hal/display.cpp` | `ledc*` (already used) | Not touched | Already the core ledc API; file is off limits (scroll optimisation work). | 0 |
| LED handling | `src/hal/rgb_led.cpp` | core digitalWrite | Kept | Active-low RGB blink state machine, no library equivalent; plain GPIO. | 0 |

## Baseline (unchanged tree, 362b3a4)

`pio test -e native`: 177 of 177 passed. cyd: flash 1,459,237 B, RAM 118,700 B. esp32-38pin: flash 1,458,725 B, RAM 118,676 B. Both are within budget (<= 1,550,000 B).
