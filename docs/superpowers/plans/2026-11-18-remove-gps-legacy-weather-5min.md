# Remove GPS and Legacy Screen Ids, Faster Weather Refresh: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. Use the `cyd-dev` skill (MCP `cyd-dev`: `cyd_gotchas`, `cyd_pinout`) before touching pins or the build. Flash and monitor with the `pio` CLI via Bash (never the platformio MCP).
>
> **Every task runs in a NEW agent (fresh context), never in the orchestrator session. Model per kind of work (user decision):**
> - **Sonnet 5.5** (`model: "sonnet"`): all code, tests, and research. It may run `pio test -e native -f <test>` inside its own TDD loop.
> - **Haiku 5.5** (`model: "haiku"`): every git action (`git add/commit/merge/push`), every verification gate (full `pio test -e native`, `pio run` of the device envs), upload to the board, and doc-only edits.
> - **Reviewer per task**: a fresh Sonnet 5.5 agent, plus one whole-branch review at the end (Task 5).
> - **Orchestrator** (main session): dispatches agents and relays results. It does not edit code.

**Goal:** Remove the Location (GPS) screen, all GPS support, and the leftover Matrix/Game screen ids, and refresh current weather every 5 minutes.

**Architecture:** Screen ids stay numerically stable for the surviving screens (stored `screen_order`, `scr_en`, `last_scr` stay valid); the removed ids 10, 11 and 13 become "not a carousel screen" through one shared predicate instead of per-id special cases. GPS code is deleted from the UART2 task, the data store, the clock, the web API and the config table. Weather only changes one interval constant.

**Tech Stack:** PlatformIO (`espressif32@6.12.0`, Arduino), LVGL 9.6, TFT_eSPI, ArduinoJson 7, Unity host tests (`env:native`).

**Spec:** the user request of 2026-11-18, as clarified in the session. Decisions:
- "GPS screen" = the Location screen (id 13). User chose **all GPS support** removed.
- "Legacy code" = the reserved Matrix (10) and Game (11) screen ids and every special case for them.
- Weather: the user's free-text answer was "current and forecast, but every 30 min". Read as: **current conditions every 5 min** (the original request), **forecast stays every 30 min**. If the user meant both at 30 min, only Task 1's constant changes.
- The bridge module's own `GPS ON` boot command (`lib/core/bridge_proto.cpp`) is protocol for external hardware, not GPS support in this firmware. It stays untouched.

## Global Constraints

- Language: code, comments, identifiers, commit messages and docs in English. PT-BR exists only as runtime UI translation.
- Commits are plain messages with **no** `Co-Authored-By` trailer and no "Generated with Claude Code" line (also for PR text).
- Screen ids that survive keep their numbers: `HamClock=1, DxCluster=2, SunSpots=3, BandInfo=4, Weather=5, AprsList=6, Pota=7, HamAlert=8, AprsRadar=9, Forecast=12, Settings=14`.
- New default `scr_en` mask: `0x13FE` (bits 1..9 and 12). New default `screen_order`: `{1, 2, 6, 9, 4, 3, 5, 12, 7, 8, 0, 0}`.
- Weather intervals: current + air quality `5UL * 60000UL`; forecast stays `30UL * 60000UL`; failure retry stays `2UL * 60000UL`.
- `lib/core` stays free of Arduino/LVGL headers.
- Every task ends with: `pio test -e native` green and `pio run -e cyd` builds (Haiku gate) before its commit.
- CI flash budget (`.github/workflows/build.yml`) must still pass; this change only shrinks the binary.

## Review Focus

- A stored `uart2_connected_module` of `"gps"` (NVS or imported backup) must become `"none"`, not leave the port in a mode the web page cannot show. Pinned in Task 3.
- A stored `last_scr`, `screen_order` or `scr_en` containing 10, 11 or 13 must boot on the Clock and clear the slot/bit, never crash or show a blank screen. Pinned in Task 2.
- An old backup JSON still containing `use_gps_if_active` or `uart2_mode` must import successfully (unknown keys ignored), not fail the whole import. Pinned in Task 3.
- With `gpsTimeValid()` gone, NTP must start whenever WiFi is up; the clock must never wait on a GPS that no longer exists. Verified on device in Task 5.
- Bridge mode used to route `$`-lines to the NMEA parser; now every non-KISS line goes to `BridgeFsm::onLine`. A `$GPRMC...` line from a bridge module must be ignored by `bridgeParseLine`, not mis-parsed. Pinned in Task 3.

## File Structure

| File | Change |
|---|---|
| `src/svc/weather.cpp` | `CUR_MS` 10 min -> 5 min (Task 1) |
| `src/ui/screens/location_screen.{h,cpp}` | delete (Task 2) |
| `src/ui/screen.h`, `registry.cpp`, `ui_manager.cpp`, `settings_screen.cpp` | drop `Matrix`, `Game`, `Location` (Task 2) |
| `lib/core/gesture.{h,cpp}` | `isCarouselScreen`, drop special cases (Task 2) |
| `lib/core/config_codec.cpp`, `config.h` | mask, default order, clamp bound (Task 2); drop GPS fields, migrate `"gps"` (Task 3) |
| `lib/core/i18n.h` | drop location-only keys (Task 2) |
| `lib/core/nmea.*`, `gps_time.*`, `test/test_nmea`, `test/test_gps_time` | delete (Task 3) |
| `lib/core/types.h`, `src/app/data_store.h`, `src/app/service_coords.cpp` | drop `GpsState` (Task 3) |
| `src/svc/uart2.{h,cpp}`, `src/net/time_service.{h,cpp}`, `src/net/web_config.cpp`, `src/ui/screens/clock_screen.cpp`, `web/config.html` | drop GPS paths (Task 3) |
| `README.md`, `docs/api.md`, `docs/screenshots/11-location.png`, `tools/check_text_fit.py`, `tools/screen_test.py` | docs and tooling (Task 4) |

## Execution waves

- Wave A (parallel, separate worktrees): Task 1, Task 2.
- Wave B: Task 3 (needs Task 2 merged: it deletes the last `GpsState` reader outside the files it edits).
- Wave C: Task 4, then Task 5.

---

### Task 1: Weather current conditions every 5 minutes

**Files:**
- Modify: `src/svc/weather.cpp:16` (`CUR_MS`)

**Interfaces:**
- Consumes: nothing.
- Produces: nothing other tasks use.

- [ ] **Step 1 (Sonnet): Change the constant.** In `src/svc/weather.cpp` set `CUR_MS = 5UL * 60000UL`. Leave `FC_MS` and `RETRY_MS` as they are. `weatherTick` already uses `s_nextCur = now + (ok ? CUR_MS : retryMs())`, so no other line changes. `weather_alerts.cpp` `CURRENT_MAX_AGE_MS` only gets looser relative to the data age; do not touch it.
- [ ] **Step 2 (Sonnet): Grep for stale mentions.** `git grep -nI '10 min\|10 minutes\|600000' -- src lib docs README.md` and fix any that describe the weather refresh interval.
- [ ] **Step 3 (Haiku): Gate.** `pio test -e native` all pass; `pio run -e cyd` builds. There is no host test for this: `weather.cpp` needs Arduino and WiFi. Cadence is verified on the board in Task 5.
- [ ] **Step 4 (Sonnet reviewer): Review the diff** against this task only.
- [ ] **Step 5 (Haiku): Commit.** `git add src/svc/weather.cpp && git commit -m "feat: refresh current weather every 5 minutes"`

---

### Task 2: Remove the Location screen and the Matrix/Game ids

**Files:**
- Delete: `src/ui/screens/location_screen.h`, `src/ui/screens/location_screen.cpp`
- Modify: `src/ui/screen.h`, `src/ui/screens/registry.cpp`, `src/ui/ui_manager.cpp`, `src/ui/screens/settings_screen.cpp`, `lib/core/gesture.h`, `lib/core/gesture.cpp`, `lib/core/config_codec.cpp`, `lib/core/config.h`, `lib/core/i18n.h`
- Test: `test/test_gesture/test_main.cpp`, `test/test_config_codec/test_main.cpp`, `test/test_i18n/test_main.cpp`

**Interfaces:**
- Consumes: nothing from earlier tasks.
- Produces:
  - `bool isCarouselScreen(uint8_t id);` in `lib/core/gesture.h`, true exactly for 1..9 and 12.
  - `enum class ScreenId : uint8_t { HamClock=1, DxCluster=2, SunSpots=3, BandInfo=4, Weather=5, AprsList=6, Pota=7, HamAlert=8, AprsRadar=9, Forecast=12, Settings=14 };`
  - `namespace scr` in `gesture.h` without `Matrix`, `Game`, `Location`.

- [ ] **Step 1 (Sonnet): Write the failing tests.**
  - `test_gesture`: replace `test_next_skips_removed_matrix` with `test_removed_ids_are_not_carousel`: `isCarouselScreen` is false for 0, 10, 11, 13, 14, 15, 200 and true for 1..9 and 12; `nextScreen` with `order = {1,10,11,13,2,0,...}` and all bits enabled returns 2 from cur 1 and 1 from cur 2; `resolveStartScreen(13, 0xFFFF)`, `(10, ...)`, `(11, ...)`, `(14, ...)` all return `scr::HamClock`. Remove the `scr::Game` assertion at the old line 27.
  - `test_config_codec`: add `test_removed_screens_cleared_by_clamp`: a `Config` from `configDefaults` with `scr_en = 0xFFFF` and `screen_order = {13,10,11,2,2,0,...}` after `configClamp` has `scr_en == 0x13FE`, `screen_order[0..2] == 0` and `screen_order[3] == 2`, `screen_order[4] == 0` (duplicate). Add `test_defaults_without_location`: `configDefaults` gives `scr_en == 0x13FE` and `screen_order == {1,2,6,9,4,3,5,12,7,8,0,0}`. Add `test_last_scr_13_rejected`: `configApplyJson` with `{"last_scr":13}` returns `ok == false` and error `bad_value:last_scr`.
  - `test_i18n`: no new test; the existing loops over `Tr::Count` must still pass after key removal.
- [ ] **Step 2 (Haiku): Run to verify failure.** `pio test -e native -f test_gesture -f test_config_codec`. Expected: compile error on `isCarouselScreen` and failing mask/order assertions.
- [ ] **Step 3 (Sonnet): Implement.**
  - `lib/core/gesture.h/.cpp`: declare and define `isCarouselScreen(uint8_t id)` as `(id >= 1 && id <= 9) || id == 12`. Use it in `carouselList` and `resolveStartScreen` in place of the `Settings/Matrix/Game/<16` checks. Drop `Matrix`, `Game`, `Location` from `namespace scr` and fix the comments in `gesture.h` that mention them.
  - `src/ui/screen.h`: new enum above, and no "reserved" comment.
  - `registry.cpp`: remove the `location_screen.h` include, `SCR_CHECK(Matrix/Game/Location)`, and the `Location` branch; the final `return createClockScreen();` stays as the fallback, with the comment reworded to "unknown id".
  - `ui_manager.cpp`: delete `normalise` and call `show`/`uiRequest` with the id directly; fix the comment at `persistScreen` ("Settings/Matrix/Game" -> "Settings").
  - `settings_screen.cpp`: drop `ScreenId::Location` from `kScreens` and `Tr::Location` from `kScreenNames`; check any other loop sized by the screen count (the `static_assert` already guards the two arrays).
  - `config_codec.cpp`: `kScrEnMask = 0x13FE`, `kDefaultOrder = {1,2,6,9,4,3,5,12,7,8,0,0}`, and the clamp loop bound `o > 13` -> `o > 12`. `config.h` comment: bits 1..9 and 12, default `0x13FE`.
  - `i18n.h`: remove `Location`, `Gps`, `GpsLabel`, `GpsActive`, `GpsInactive`, `Fix`, `Sats`, `Lat`, `Lon`, `Speed`, `Locator`, `Saved`, `NoFix`, `Save`, `Yes`, `No` only after `git grep -n 'Tr::<Key>\b' -- src` shows no remaining user (the clock screen still uses `Tr::Gps` until Task 3, so leave `Gps` for Task 3). Keep `KmH` (alert overlay) and `WeatherForLocation`.
  - Delete the two `location_screen` files.
- [ ] **Step 4 (Haiku): Run tests.** `pio test -e native` all pass.
- [ ] **Step 5 (Haiku): Build.** `pio run -e cyd` and `pio run -e esp32-38pin` build; if the build reports `Tr::<Key>` still used, restore that key.
- [ ] **Step 6 (Sonnet reviewer): Review the diff.** Check no stale `Matrix`, `Game` or `Location` references: `git grep -nIw 'Matrix\|Game\|Location' -- src lib test` returns nothing relevant.
- [ ] **Step 7 (Haiku): Commit.** `git add -A src lib test && git commit -m "refactor: remove the Location screen and the reserved Matrix/Game screen ids"`

---

### Task 3: Remove all GPS support

**Files:**
- Delete: `lib/core/nmea.{h,cpp}`, `lib/core/gps_time.{h,cpp}`, `test/test_nmea/`, `test/test_gps_time/`
- Modify: `lib/core/types.h`, `lib/core/config.h`, `lib/core/config_codec.cpp`, `lib/core/i18n.h`, `src/app/data_store.h`, `src/app/service_coords.cpp`, `src/svc/uart2.{h,cpp}`, `src/net/time_service.{h,cpp}`, `src/net/web_config.cpp`, `src/ui/screens/clock_screen.cpp`, `web/config.html`
- Test: `test/test_config_codec/test_main.cpp`, `test/test_bridge/test_main.cpp`, `test/test_ring_buffer/test_main.cpp`

**Interfaces:**
- Consumes: Task 2 (no remaining `GpsState` reader in `src/ui/screens/location_screen.cpp`).
- Produces:
  - `Config` without `gps_auto_pos` and `uart2_mode`.
  - `serviceCoords(double& lat, double& lon)` unchanged signature; now returns user lat/lon, else locator.
  - `uart2.h` without `gpsTimeValid()`; `time_service.h` without `ntpSyncedWithin`.

- [ ] **Step 1 (Sonnet): Write the failing tests.**
  - `test_config_codec`: `test_gps_module_migrates_to_none`: set `c.uart2_cmod = "gps"`, `configClamp(c)`, assert `uart2_cmod` equals `"none"`; and `configApplyJson(c, "{\"uart2_connected_module\":\"gps\"}")` is ok and leaves `"none"`. `test_old_backup_gps_keys_ignored`: `configApplyJson(c, "{\"use_gps_if_active\":true,\"uart2_mode\":\"gps\",\"user_callsign\":\"PY2XYZ\"}")` is ok and `user_callsign == "PY2XYZ"`. Remove any existing assertion on `gps_auto_pos`, `use_gps_if_active` or `uart2_mode` and any field-count assertion that changes.
  - `test_bridge`: `test_bridge_ignores_nmea_lines`: `bridgeParseLine("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A")` returns `BR_NONE`. The boot `GPS ON` assertions stay as they are.
  - `test_ring_buffer`: `test_types_pod` drops `GpsState` from its `static_assert`.
- [ ] **Step 2 (Haiku): Run to verify failure.** `pio test -e native -f test_config_codec`. Expected: FAIL on the migration test (`"gps"` is kept today).
- [ ] **Step 3 (Sonnet): Implement `lib/core` changes.**
  - Remove `GpsState` from `types.h`; remove `gps_auto_pos` and `uart2_mode` from `config.h`; remove the `use_gps_if_active` and `uart2_mode` rows from `kFields` and the `uart2_mode` default in `configDefaults`.
  - In `configClamp`, after the field loop: `if (!strcmp(c.uart2_cmod, "gps")) strcpy(c.uart2_cmod, "none");`.
  - Delete the nmea and gps_time sources and their test dirs. Remove `Tr::Gps` from `i18n.h`.
  - Check `config_store.cpp` loads by `nvsKey` over `kFields`, so orphaned NVS keys are simply ignored; do not add cleanup code.
- [ ] **Step 4 (Sonnet): Implement firmware changes.**
  - `data_store.h`: remove `Guarded<GpsState> gps`.
  - `service_coords.cpp`: remove `gpsAuto`, the `GpsState` read and the `if (f.gpsAuto)` block; update the comment in `service_coords.h` to "user lat/lon, then locator".
  - `uart2.cpp`: remove `M_GPS`, the `"gps"` case in `modeOf`, `syncClock`, `g_timeSetMs`, `gpsTimeValid`, the `nmea.h`/`gps_time.h`/`net/time_service.h`/`sys/time.h` includes if unused, the `GpsState gps, pub`, `lastSentence`, the `line[0] == '$'` NMEA branch (bridge lines fall through to `BridgeFsm::onLine`), the `M_GPS || M_BRIDGE` block, and the final publish block. Keep the line assembly for `M_TNC` and `M_BRIDGE`. `uart2.h`: drop `gpsTimeValid` and fix the task comment.
  - `time_service.cpp/.h`: delete `if (gpsTimeValid()) return;`, the `uart2.h` include if now unused, `ntpSyncedWithin` and its declaration; fix the `timeTick` comment ("skipped while GPS time is valid").
  - `web_config.cpp`: remove `GpsState g` and the `g_store.gps.read` from `handleStatus` and `handleUartStatus`, and the `gps` / `gps_active` / `gps_fix` keys.
  - `clock_screen.cpp`: remove the `gps_` label, `lastGps_`, the `GpsState` read and the `gpsFix` block.
  - `web/config.html`: remove the `use_gps_if_active` row and its PT string, the `["gps","GPS"]` option, the `GPS fix` status text and its PT string (keep the status line otherwise intact: `s.gps` no longer exists, so it must not be dereferenced), and rename the section `UART2 / GPS / bridge` to `UART2 / bridge` (EN key and PT translation). `tools/gen_web.py` regenerates the embedded page at build time.
- [ ] **Step 5 (Haiku): Run tests.** `pio test -e native` all pass.
- [ ] **Step 6 (Haiku): Build all envs.** `pio run -e cyd`, `pio run -e esp32-38pin`, `pio run -e cyd_test` build with no warnings about unused GPS symbols.
- [ ] **Step 7 (Sonnet reviewer): Review the diff.** `git grep -niI 'gps\|nmea' -- src lib test web` must show only the bridge `GPS ON` command, its test, and the `$GPRMC` rejection test.
- [ ] **Step 8 (Haiku): Commit.** `git add -A lib src test web && git commit -m "refactor: remove GPS support (UART2 GPS mode, NMEA parser, GPS time sync, clock tag, auto-position)"`

---

### Task 4: Docs and tooling cleanup

**Files:**
- Modify: `README.md`, `docs/api.md`, `tools/check_text_fit.py`, `tools/screen_test.py`
- Delete: `docs/screenshots/11-location.png`

**Interfaces:**
- Consumes: Tasks 2 and 3 (final key names and ids).
- Produces: nothing.

- [ ] **Step 1 (Sonnet): Tooling.** In `tools/check_text_fit.py` delete the `GPS_END` line and its Clock entry (restore a plain clock IP row entry without the GPS-tag width) and the whole Location block (`LOC`, three `add("Location", ...)` calls). In `tools/screen_test.py` remove `("location", 13)` from the screen list and make sure no logic depends on its index.
- [ ] **Step 2 (Haiku): Verify tooling.** `python3 tools/check_text_fit.py` exits 0.
- [ ] **Step 3 (Haiku): Docs.**
  - `README.md`: remove the Location bullet (line ~24) and the Location screenshot cell (line ~47; rebalance the table row), and any GPS mention; delete `docs/screenshots/11-location.png`.
  - `docs/api.md`: remove `use_gps_if_active` and `uart2_mode` rows; rename the section heading `UART2 / GPS / bridge` to `UART2 / bridge`; the module list drops `gps`, with a note that a stored `gps` value is converted to `none`; `/api/status` and `/api/uart_status` examples and descriptions lose the `gps*` keys; `screen_order` default and range become `[1,2,6,9,4,3,5,12,7,8,0,0]` and `0 .. 12`; `scr_en` default `5118` (`0x13FE`) and the clamp range copied from the `scr_en` row in `kFields` as it stands after Task 2 (bits outside `0x13FE` are always cleared); the bit list drops 13 Location; the `last_scr` text drops "Matrix or Game"; the "76 table fields" count drops by 2 (verify against `kFieldCount`); remove "GPS auto-position" from the station list.
  - Leave `docs/review/**` and `docs/port-notes/**` untouched: they are historical records.
- [ ] **Step 4 (Haiku): Commit.** `git add -A README.md docs tools && git commit -m "docs: drop the Location screen and GPS from README, API reference and tools"`

---

### Task 5: Whole-branch review, device verification, push

**Files:** none (verification only; any fix goes back to a Sonnet agent with a new commit).

**Interfaces:**
- Consumes: Tasks 1-4 merged on `main`.
- Produces: pushed `main`.

- [ ] **Step 1 (Haiku): Full gate.** `pio test -e native` all pass; `pio run -e cyd`, `-e esp32-38pin`, `-e cyd_test` build. Report the flash size line from the `cyd` build against the CI `FLASH_BUDGET`.
- [ ] **Step 2 (Sonnet, fresh agent): Whole-branch review.** Run the `code-review` skill (or `superpowers:requesting-code-review`) over `627ec4c..HEAD`. It checks each Review Focus line above against the code, and that nothing else reads a removed `Config` field or `Tr` key at runtime (for example through `kFields` offsets). Findings go to a new Sonnet agent to fix, then Haiku re-runs the gate.
- [ ] **Step 3 (Haiku): Flash.** With the board on USB: `pio run -e cyd -t upload`. If the port is missing, report it and stop; do not guess a port.
- [ ] **Step 4 (Haiku): Capture 12 minutes of serial.** `timeout 720 pio device monitor --filter time > <scratchpad>/mon.log`. Expected: `NTP synced` boot line appears; two `[HTTP] GET api.openweathermap.org -> 200` pairs (weather + air_pollution) roughly 5 minutes apart; no `GPS` log text; no reset or `Guru Meditation`. Requires an OpenWeather key in the device config; if none is set, report that the cadence could not be observed and stop.
- [ ] **Step 5 (user, via orchestrator): Visual check.** Swipe through the carousel (no Location screen), confirm the clock has no GPS tag, and open the web page (no GPS fields, UART module list without GPS).
- [ ] **Step 6 (Haiku): Push.** `git push origin main`.

---

## Self-Review

- **Spec coverage:** Location screen (Task 2), all GPS support (Task 3), Matrix/Game ids (Task 2), weather 5 min (Task 1), docs/tools (Task 4), cyd-dev skill and board-level checks (header, Task 5), new agents and model split (header, every step labeled), review (per-task reviewer plus Task 5).
- **Type consistency:** `isCarouselScreen(uint8_t)` is defined in Task 2 and not used elsewhere by name. Default `scr_en` `0x13FE` and default order match across Global Constraints, Task 2 tests and Task 4 docs.
- **Open point for the user:** the weather reading above (current 5 min, forecast 30 min).
