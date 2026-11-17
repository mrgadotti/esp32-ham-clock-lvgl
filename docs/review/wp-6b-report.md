# WP6b report: Net sockets and scheduling

Findings: N-01, N-02, M-06 (diagnostics + allocation order), K-01, K-04, plus the WP8 migrations and dead-field cleanup.

## What changed
- **N-01 / K-01.** New `src/svc/async_connect.{h,cpp}`: DNS through the lwIP callback (`dns_gethostbyname_addrtype`, the same call `WiFi.hostByName` makes, 5 s timeout) and `connect()` on a non-blocking socket polled with `select()` (4 s timeout), then handed to `WiFiClient(fd)`. No call blocks. New `src/svc/line_reader.h`: the line splitter shared by DX, APRS-IS and HamAlert.
  - HamAlert is a state machine (Connecting, Banner, Login, Read). The 180/220/160 ms gaps between commands are timestamps, not `delay()`. It no longer allocates a heap `Work`: spots live in `spotScratch()`. The scratch is held across ticks through `spotScratchHeld()`, and POTA skips its tick while it is held.
  - DX and APRS-IS connect through `AsyncConnect` too, so a failed connect no longer stalls the net task for 4 s (or 15 s on DNS).
  - HTTPS fetches (weather, POTA, QRZ, propagation) still block the net task for up to 8 s each. The report's `netPump()` idea needs `http_fetch.cpp` and is not done here (WP6a file, out of scope).
- **N-02.** The UI loop sleeps for the time `lv_timer_handler()` returns, clamped to 1..5 ms.
- **K-04.** `uart2BridgeStatus` copies the fields into a static POD under `g_mu` and builds the JSON after releasing it.
- **C-14.** `g_mode`, `g_pairPending` and `g_timeSetMs` in `uart2.cpp` are `std::atomic`.
- **WP5 API.** DX, APRS-IS, HamAlert and the beacon keep a narrow config struct refreshed only when `cfgVersion()` changes. No per-tick `Config` copy.
- **Migrations.**
  - `uart2.cpp`: local `daysFromCivil` and date checks replaced by `gpsToUnix`.
  - `qrz.cpp`: local `urlEncode` replaced by `url_encode.h`.
  - `dx_cluster.h` no longer re-declares `qrzOnNewSpot`: the .cpp includes `svc/qrz.h`.
  - `kDxSpots` was already in use.
- **Dead fields.** `Spot.lat/lon/hasLatLon` had only writers (grep: the radar and alert readers use `AprsStation`). Removed from `types.h`, qrz/dx/hamalert, `dx_parse.cpp` and one test assert. "spotter" dropped from the POTA filter.
- `rgb_led.cpp` needed no change: it is already atomic, and with a non-blocking HamAlert the blink is no longer frozen.

## M-06: heap diagnostics and allocation order
- `MEM_DIAG_ONCE(tag)` (`app/mem_log.h`, on by default, `-DMEM_DIAG=0` removes it) prints `[MEM] <tag> free largest allocs freeblocks` once per call site. Sites: before/after the first HTTPS GET (`http_fetch.cpp`), and before/after the first connect of DX, APRS-IS and HamAlert.
- The net task now runs the HTTPS services first (POTA, QRZ, weather, propagation). DX, APRS-IS and HamAlert may only connect 3 s after the first HTTPS round. The round ends on the tick where the clock is valid (that tick also runs the first propagation fetch), with a 15 s fallback after STA is up. The gate resets when STA drops.
- Persistent buffers (HamAlert line buffer and spot scratch, bridge status snapshot) are static, so they are placed at link time, before WiFi and TLS. The display buffer, DataStore, QRZ mutex and web server already come up before WiFi.
- **mbedTLS buffer sizes.** Not possible here. The precompiled sdkconfig has `CONFIG_MBEDTLS_SSL_MAX_CONTENT_LEN=16384` and `MBEDTLS_ASYMMETRIC_CONTENT_LEN` unset. `WiFiClientSecure` 2.0.17 has no `setBufferSizes`. Changing them needs a custom ESP-IDF libs build, outside this package.
- Expected effect: the TLS handshakes of the first round run before any DX/APRS/HamAlert socket exists, so the first large I/O buffers land in the still-contiguous heap.

## Numbers
- `cyd`: RAM 116476 -> 118612 B, flash 1737505 -> 1742313 B.
- `esp32-38pin`: RAM 118588 B, flash 1741773 B.
- The baseline build was taken at the start of work and may already include a few early edits.
- `pio test -e native`: 170/170. `pio run -e cyd` and `pio run -e esp32-38pin` pass.

## Hardware checks still needed
- `/api/status` p99 < 200 ms while HamAlert polls, and an LED blink with the correct period during a poll.
- `[MEM]` lines around the first connects, and `largest >= 45 KB` at idle after the first round.
- No `SSL - Memory allocation failed (-32512)` in 10 consecutive boots.
- The HamAlert list still fills (the server's non-ASCII bytes are now dropped by `LineReader`, as DX always did).
- DX connect/reconnect with a bad host or DNS failure, to confirm the net task keeps running.

## Review fixes
- **TLS guard** (`lib/core/tls_policy.{h,cpp}`, native test `test_tls_policy`; `httpPollGate()` in `http_fetch.cpp`). Every HTTPS poller (POTA, QRZ, weather, propagation) calls the gate before its fetch. It returns false (nothing consumed, asked again next tick) while HamAlert holds the line, or when `memLargest8()` is below `kTlsMinBlock` (28 KB). In that case it logs `[HTTP] deferred (largest=...)` and waits 2 s. After 5 deferrals it goes anyway, so it never blocks permanently.
- **Quick retry.** The SSL allocation failure is detected from `WiFiClientSecure::lastError()` (-32512). `httpQuickRetryMs()` returns 4000 once; the token is restored by the next HTTP 200. POTA, weather and propagation use it instead of their long retry. QRZ keeps its own queue backoff.
- **HamAlert.** `hamalertBusy()` (state != Idle) feeds the gate. If a tick arrives more than 2 s late, the session is marked cut and `finish()` keeps the previous list. The ring uses `kSpotScratchN`.
- **Diagnostics.** `MEM_DIAG` is off by default (enable with `-DMEM_DIAG=1`), and the first fetch also logs "after end".
- **APRS-IS.** It drops and reconnects when host, port, callsign, passcode, SSID or radius change.
- **DX.** The backoff base is the end of the failed attempt (APRS-IS already counted from the failure tick).
- **Numbers.** `cyd` RAM 118732 B, flash 1742433 B. `esp32-38pin` RAM 118708 B, flash 1741909 B. Native 173/173.
- **Hardware.** The 28 KB threshold is a first value. Check the `[HTTP] deferred` frequency on the board and tune `kTlsMinBlock`.
