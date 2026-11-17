# ESP32-HAM-CLOCK → LVGL + PlatformIO — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: `superpowers:subagent-driven-development`. Steps use checkbox (`- [ ]`) syntax. Use the `cyd-dev` skill (MCP `cyd-dev`) for pins, gotchas and scaffolding. Flash and monitor with the `pio` CLI via Bash (never the platformio MCP).
>
> **Model per kind of work (user decision):**
> - **Sonnet 5.5**: all code (firmware, tests, build scripts, CI workflows, flasher page). It may run `pio test -e native -f <test>` inside its own TDD loop.
> - **Haiku 5.5**: all documentation (README, LICENSE, NOTICE, checklists, budget notes) and each task's **verification gate**: full `pio test -e native`, `pio run` of both device envs, upload/monitor, and `git add/commit/push`.
> - **Reviewer per task**: Sonnet 5.5.
> - **Orchestrator** (main session): dispatches agents, merges worktrees, and runs the visual confirmations with the user.
> - Run agents in parallel wherever the "Execution waves" section allows.

**Goal:** Rewrite ESP32-HAM-CLOCK (an 18,615-line TFT_eSPI monolith) as a new modular PlatformIO + LVGL 9 project: every screen ported, an on-device Settings screen, a config-only web page, lean RAM/flash use, CI builds and a browser flasher.

**Architecture:** Three layers.
- `lib/core/` holds pure C++ with no Arduino dependency (parsers, geo, calibration, codecs), unit-tested on the host with Unity (`env:native`).
- `src/svc/` and `src/net/` hold FreeRTOS services that publish into a `DataStore` (one mutex plus a version counter per domain).
- `src/ui/` is LVGL, touched only by the UI task, with one `Screen` class per screen, created on entry and destroyed on exit.

Data structs use fixed `char[]` buffers (no `String`), so there is no heap churn.

**Tech Stack:**
- PlatformIO with `platformio/espressif32@6.12.0` (Arduino 2.0.17).
- LVGL 9.x, pinned to the exact version `cyd_new_project` resolves.
- TFT_eSPI 2.5.43 with an **own flush callback**: `display.cpp` owns the `TFT_eSPI` object and uses `lv_display_create` + `lv_display_set_flush_cb` with `pushColors` and byte swap. Do not use `lv_tft_espi_create`, which hides the object and blocks runtime invert/rotation.
- XPT2046_Touchscreen on VSPI, ArduinoJson 7, Unity (native).
- `lv_font_conv` (npx) and Python 3 for build scripts.

**Spec:** the decisions from this session's grilling, summarized under Global Constraints. Analysis notes on the original code are copied in Task 1 to `docs/port-notes/` (`screens_A.md`, `screens_B.md`, `services.md`, `config_web.md`, `cyd_lvgl.md`). The original code is read-only at `/home/marcelo/dev/embedded/esp32/cyd/ESP32-HAM-CLOCK/src/ESP32-HAM-CLOCK.cpp`; tasks cite the lines to port.

## Execution kickoff (orchestrator, before Task 1)

1. Copy this plan to `/home/marcelo/dev/embedded/esp32/cyd/esp32-ham-clock-lvgl-plan.md`, as the user asked. Task 1 later also copies it into the repo, at `docs/superpowers/plans/2026-11-16-lvgl-migration.md`.
2. Load `superpowers:subagent-driven-development` and run the waves below, starting with wave A.
3. Write `include/secrets.h` (gitignored) as soon as Task 1 has created the project folder.

## Global Constraints

**Project and repository**
- New project at `/home/marcelo/dev/embedded/esp32/cyd/esp32-ham-clock-lvgl/`, in its own git repo. The original project is never modified.
- Remote `origin` = `git@github.com:mrgadotti/esp32-ham-clock-lvgl.git` (SSH), branch `main`. The first push is `git push -u origin main`, and every task ends with a push to `main`.
- **Language:** all code, comments, identifiers, commit messages and documentation (README, NOTICE, docs/, this plan, the web page source) are in English. PT-BR exists only as a runtime UI translation (i18n tables on the TFT and the web page).
- Commits are plain English messages with **no** `Co-Authored-By` trailer and no "Generated with Claude Code" line.
- **License:** GPLv3 in `LICENSE` (the full official text). The original is MIT (© 2026 Konrad Wisniewski SP3KON); that notice is kept in `NOTICE` and in the README credits, as MIT requires.
- **CI/CD:** GitHub Actions builds both device envs and runs the native tests on every push and PR. A Pages workflow publishes an ESP Web Tools flasher at `https://mrgadotti.github.io/esp32-ham-clock-lvgl/`.
- `secrets.h` is optional: the code uses `#if __has_include("secrets.h")` and falls back to an empty SSID, which leads to the AP fallback. CI builds therefore need no secrets.

**Boards and pins**
- Envs (**`cyd` is the default env**: `default_envs = cyd`; the web flasher lists it first, and the README examples use it):
  - `cyd`: ESP32-2432S028R with ILI9341, single USB.
  - `esp32-38pin`: ESP32 38-pin DevKit with an external ILI9341.
  - `native`: host tests.
- `cyd` pins:
  - TFT on HSPI: MISO12, MOSI13, SCLK14, CS15, DC2, RST -1.
  - Backlight 21 (PWM, active HIGH).
  - Touch on VSPI: CLK25, MOSI32, MISO39, CS33, IRQ36.
  - RGB LED: R4, G16, B17 (active LOW).
  - UART2: RX27, TX22.
- `esp32-38pin` pins:
  - TFT the same as `cyd`.
  - Touch: CLK25, MOSI32, **MISO27, IRQ35**, CS33.
  - **UART2: RX16, TX17.**
  - No RGB LED (`HAS_RGB_LED` undefined).
- Display driver: `ILI9341_2_DRIVER` + `TFT_INVERSION_ON` are the build default on both envs. Inversion can also be toggled at runtime (`tft.invertDisplay`) from Settings (NVS `tft_inv`). `TOUCH_CS` is **not** defined for TFT_eSPI.

**Flash and partitions**
- Partition table `huge_app.csv`: 3 MB app, no OTA.
- **No LittleFS:** fonts, icons and the web page are compiled into the firmware.
- **Flash erase:**
  - The first time this project is flashed onto a board, run `pio run -e <env> -t erase` first, which clears NVS and leftovers from other projects.
  - Later uploads do not erase, so calibration and WiFi are kept, unless a task says otherwise.
  - The web flasher offers erase (`new_install_prompt_erase: true`).
  - The README documents `pio run -e cyd -t erase && pio run -e cyd -t upload`.

**Budgets**
- App flash ≤ 2.6 MB.
- Static RAM (`pio run`) ≤ 150 KB. Task 1 records the real baseline.
- Soak heap (`[MEM]` log every 10 s): largest free block ≥ 45 KB, measured at idle between fetches; `minfree` watermark ≥ 30 KB.

**LVGL**
- `LV_MEM_SIZE` 40 KB.
- Two static 320×10 px RGB565 draw buffers (12.8 KB total).
- `lv_tick_set_cb(millis)`.
- `LV_USE_LOG 0` in release builds.
- `LV_USE_FONT_COMPRESSED 0`.

**Fonts**
- Montserrat generated with `lv_font_conv --no-compress`, bpp 4:
  - range `0x20-0x7F,0xA0-0xFF` (Latin-1) at sizes **14** (tables and body), **18** (titles) and **24** (big values);
  - clock font at **48** px with only the symbols `0123456789:-`.
- Built-ins: only `LV_FONT_MONTSERRAT_14` stays enabled, as `fallback` of the custom fonts, because dropdown, checkbox and list widgets need the `LV_SYMBOL_*` glyphs.

**Tables** (DX, POTA, HamAlert, APRS)
- **One `lv_table`** per screen, 14 px font, 17 px rows (minimal padding), giving about 11 visible rows.
- Per-column colors are applied in `LV_EVENT_DRAW_TASK_ADDED`, because one label per cell would overflow `LV_MEM`.
- Vertical drag scroll (`LV_DIR_VER`), no horizontal scroll.
- Rows are read one at a time through a callback under the store lock, never as a whole array on the stack.

**Navigation**
- A horizontal swipe changes screen (left = next, right = previous), with wrap.
- The gesture is detected in the **indev read callback** in `touch.cpp`. It records the press origin, calls `classifyGesture(dx,dy)` on release, and posts the result to `UiManager`. This is needed because `LV_EVENT_GESTURE` does not fire over scrollable children.
- No gear icon and no long-press menus.
- Screens are created on entry and destroyed on exit, **deferred**: `UiManager::tick` applies a pending switch, never inside the screen's own event.

**Settings screen**
- Always present, pinned last in the carousel, cannot be disabled.
- Skipped by auto-switch, and auto-switch pauses while it is open. Auto-switch never moves to Settings: after the last enabled carousel screen it wraps to the first one (`nextAutoSwitchScreen`). Swipe still reaches Settings.

**Effective screen list**
- `screen_order` slots with a non-zero code **and** their bit set in `scr_en`, then Settings last.
- "Off" in the web page writes 0 to the slot.

**Rotation**
- `tft_rot` is 1 (0°) or 3 (180°); any other stored value becomes 1.
- At 180° the touch coordinates are mirrored (`x=319-x`, `y=239-y`) after calibration, so `tcal` stays valid.

**Languages**
- TFT and web support EN and PT-BR, **default EN**.
- Strings live in `const` flash tables.
- `enum class Lang : uint8_t { EN = 0, PT_BR = 1 }`; `langFromCode` accepts `"en"`, `"pt-br"` and `"pt_br"`.

**Theme**
- Dark only.
- Removed: light theme, hue slider and the "enlarged table" mode.

**Web**
- A single config page, served as embedded gzip, with EN/PT-BR switching in JS.
- Excluded: spots, APRS, HamAlert, propagation, links and the TFT mirror.
- `GET /api/config` never returns secrets; it returns `<field>_set: true/false` instead.
- `POST /api/save` is partial (only the keys present), and an empty password keeps the current one.
- No auth (LAN use).
- AP fallback: SSID `ESP32-HAM-CLOCK`, default password `1234567890`, configurable (`ap_pass`).

**NVS**
- New namespace `hamclk`; no compatibility with the old firmware's NVS (the flash is erased).
- Key names follow the original where one exists, because the web page and the JSON backup use the same names.
- New keys:
  - `tcal`: 6 floats, the affine calibration;
  - `aprs_sort`;
  - `scr_en`: uint16, bit = `ScreenId`, defaulting to all 12 bits of screens 1–10, 12 and 13 set.

**Config access across tasks**
- `cfgGet(Config& out)` copies the config under a mutex; services take one copy per tick.
- Only the UI task uses the `cfg()` reference, which is refreshed on the `ConfigChanged` event.

**Service coordinates**
- `bool serviceCoords(double& lat, double& lon)`, defined in Task 6.
- Priority: GPS (if `use_gps` and fix ≤ 15 s old; this branch is enabled in Task 17), then `user_lat/lon`, then the locator.

**Network text**
- Parsers reduce comments and names to printable ASCII and drop control bytes, so labels never get invalid UTF-8.

**Test WiFi**
- `include/secrets.h` is gitignored and holds `SECRET_WIFI_SSID "NASA"` plus the password the user gave in this session. The orchestrator writes this file; it is never committed.
- It is only the default when NVS `wifi_ssid` is empty. IP is assigned by DHCP.

**Concurrency (FreeRTOS)**
- `ui` task: core 1, priority 3, 8 KB stack. It is the only task that calls LVGL.
- `net` task: core 0, priority 1, 10 KB stack. WiFi, WebServer, telnet and HTTPS run sequentially in it.
- `uart` task: core 1, priority 2, 4 KB stack, with a 1 KB UART2 RX buffer.
- Alerts reach the UI through an `xQueue` of POD structs.
- `loop()` calls `vTaskDelete(NULL)`.

**Data**
- Structs use fixed `char[]` buffers.
- Fixed ring buffers: `dx` 100, `pota` 30, `hamalert` 30, `aprs` 30, `qrzCache` 20.
- No `String` in `lib/core` or in the stores.

**Original bugs to fix**
- VHF/UHF/SHF filters compare kHz values as MHz.
- No 30m tile.
- HamAlert clears its list on a failed poll.
- The APRS sort is not persisted.
- The APRS menu CLOSE button discards the checkboxes.
- "Repeat: N min" actually shows hours.
- The "BECONING" and "WIATR" typos.
- The default APRS passcode `00000` is an octal literal.
- `/api/save` wipes absent fields.
- `pota_port` is reset on every save.

**Removed**
- Build scripts: `erase_fs.py`, `copy_user_setup.py`, `patch_tftespi_smooth_font.py`.
- The POTA telnet path, the radar view toggle and the Matrix intro.
- The light theme and the 15 `*w.bmp` icons.
- `logo.bmp` (the splash becomes text).
- `openwebrx_url` and the Links tab, the `.txt` manuals, OTA and LittleFS.

## Review Focus

1. **WiFi or a server drops mid-session.**
   - Expected: screens keep the last data with its age (an "updated 12 min ago" footer on Propagation, Weather and Forecast, from `fetchedAt`), and services reconnect with backoff without freezing the UI.
   - Tests: Task 4 `test_backoff`, plus the hardware checklist in T6 and T21 (power off the router).
2. **Heap fragmentation after hours of busy APRS-IS.**
   - Expected: the largest free block at idle stays ≥ 45 KB, enough for the TLS handshake.
   - Test: the Task 21 2-hour soak with the `[MEM]` log.
3. **A bad calibration makes touch unusable.**
   - Expected: holding the screen for 2 s at boot forces recalibration, and calibration rejects a result whose verify error is > 8 px.
   - Test: Task 2 `test_cal_rejects_collinear` and its verify step.
4. **A horizontal swipe is confused with a vertical list scroll.**
   - Expected: a swipe needs |dx| > 40 px and |dx| > 2·|dy|, and lists use `LV_DIR_VER`.
   - Tests: Task 4 `test_gesture` and the Task 7 checklist.
5. **Malformed network payloads** (truncated telnet line, APRS garbage, partial JSON).
   - Expected: the parser returns `false`, nothing is inserted, and nothing crashes.
   - Tests: the `*_rejects_garbage` cases in Tasks 10, 12, 13, 17, 18 and 19.

---

## Target file structure

```
esp32-ham-clock-lvgl/
  platformio.ini                 [env] common, [esp32_base], envs cyd, esp32-38pin, native
  include/lv_conf.h  board.h  secrets.h.example
  tools/gen_web.py               pre-script: web/config.html → src/net/config_html.h (gzip, PROGMEM)
  tools/gen_fonts.sh             lv_font_conv → src/ui/fonts/*.c
  tools/gen_icons.py             50x50 BMP → src/ui/icons/*.c (RGB565 lv_image_dsc_t)
  tools/merge_bin.py             merged image for the web flasher
  web/config.html
  flasher/index.html  flasher/manifest-*.json
  .github/workflows/build.yml  pages.yml
  docs/port-notes/*.md  docs/budget.md  docs/hardware-checklist.md
  docs/superpowers/plans/2026-11-16-lvgl-migration.md
  LICENSE  NOTICE  README.md
  lib/core/                      PURE: types, geo, band_mode, ring_buffer, backoff, gesture,
                                 touch_cal, i18n, config, config_codec, dx_parse, pota_parse,
                                 hamalert_parse, aprs_parse, aprs_wx, alert_rules, radar_proj,
                                 ax25_kiss, nmea, bridge_proto, propagation_xml, weather_json,
                                 weather_alerts, qrz_cache
  src/main.cpp
  src/app/   config_store  data_store  events  mem_log  service_coords
  src/hal/   display  touch  touch_cal_ui  rgb_led
  src/net/   wifi_manager  web_config  time_service  http_fetch
  src/svc/   dx_cluster  qrz  pota  hamalert  aprs_is  aprs_beacon  uart2  propagation  weather
  src/ui/    ui_manager  screen.h  theme  widgets/(header, spot_table, filter_menu)
             overlays/alert_overlay
             screens/(registry, boot, clock, dx, pota, hamalert, aprs_list, aprs_radar,
                      propagation, bands, weather, forecast, matrix, game, location, settings)
  test/test_<module>/test_main.cpp
```

### Core interfaces (defined in Tasks 4–7, used everywhere)

```cpp
// lib/core/types.h  (Task 4 — ALL POD data structs live here)
struct Spot { uint32_t utc; uint32_t khz; char call[12]; char spotter[12]; char comment[40];
              char country[24]; char loc[7]; float lat, lon; int16_t distKm; uint8_t band, mode;
              bool hasLatLon; };                                   // DX, POTA, HamAlert
struct AprsStation { uint32_t utc; char call[12]; char sym[2]; char comment[48]; float lat, lon;
                     float freqMhz; int16_t distKm; int16_t bearing; bool hasLatLon; bool isWx;
                     char source; };                                // 'I' APRS-IS, 'T' TNC2, 'K' KISS
// + AprsWx, PropagationData, WeatherData (with uint32_t fetchedAt), WeatherAlertInfo, GpsState,
//   NetStatus, BeaconInfo, QrzInfo, AlertCfg, WxAlertCfg — fields as listed in Tasks 11–19.
template<typename T, size_t N> class RingBuffer {   // newest-first, fixed storage
 public: void pushFront(const T&); size_t size() const; const T& at(size_t i) const; void clear();
         template<class Pred> bool moveToFrontIf(Pred, const T& replacement); };

// src/app/data_store.h  (Task 6)
template<typename T> class Guarded { public: template<class F> void write(F f);   // bumps version
                                     template<class F> void read(F f) const; uint32_t version() const; };
struct DataStore { Guarded<RingBuffer<Spot,100>> dx; Guarded<RingBuffer<Spot,30>> pota, hamalert;
                   Guarded<RingBuffer<AprsStation,30>> aprs; Guarded<PropagationData> prop;
                   Guarded<WeatherData> weather; Guarded<GpsState> gps; Guarded<NetStatus> net; };
extern DataStore g_store;
// src/app/events.h  (Task 6)
enum class EventKind : uint8_t { AprsAlert, Beaconing, WeatherAlert, ConfigChanged, StartTouchCal };
struct UiEvent { EventKind kind; union { AprsStation station; BeaconInfo beacon; WeatherAlertInfo wx; }; };
bool postUiEvent(const UiEvent&);   // xQueueSend, timeout 0
bool popUiEvent(UiEvent&);
// src/ui/screen.h  (Task 7)
enum class ScreenId : uint8_t { HamClock=1, DxCluster=2, SunSpots=3, BandInfo=4, Weather=5, AprsList=6,
  Pota=7, HamAlert=8, AprsRadar=9, Matrix=10, Game=11, Forecast=12, Location=13, Settings=14 };
class Screen { public: virtual ~Screen() = default; virtual ScreenId id() const = 0;
  virtual void create(lv_obj_t* parent) = 0;   // build widgets
  virtual void tick(uint32_t nowMs) = 0;        // every 100 ms; refresh only if a store version changed
  virtual bool blocksSwipe() const { return false; } };  // true while a menu/modal/game is active
Screen* createScreen(ScreenId);                 // factory in src/ui/screens/registry.cpp
```
`ScreenId` keeps the original numbering, so the 12-byte `screen_order` keeps its meaning in the web page and the JSON backups.

---

### Task 1: Scaffold, display and color inversion on hardware

**Files:**
- Create: `platformio.ini`, `include/lv_conf.h`, `include/board.h`, `include/secrets.h.example`, `.gitignore`, `src/main.cpp`, `src/hal/display.{h,cpp}`, `tools/gen_web.py` (stub), `docs/port-notes/*`, `docs/budget.md`, `docs/superpowers/plans/2026-11-16-lvgl-migration.md`, `test/test_smoke/test_main.cpp`

**Interfaces — Produces:**
- `void displayInit(uint8_t rotation, bool invert)`
- `void displaySetBacklight(uint8_t percent)`
  - LEDC channel 0, 5 kHz, 8 bit, clamped to 5–100.
  - Uses the Arduino 2.0.17 API `ledcSetup(0,5000,8)` + `ledcAttachPin(PIN_BL,0)` + `ledcWrite(0,duty)`, not `ledcAttach`.
- `void displaySetInvert(bool)`
- `void displaySetRotation(uint8_t rot)` (1 or 3)
- `board.h` macros: `PIN_BL`, `PIN_TOUCH_*`, `PIN_UART2_RX/TX`, `HAS_RGB_LED`

- [ ] **Step 1: Scaffold the project.**
  - `git init`, then scaffold with the MCP `cyd_new_project` (board `cyd-2432s028r`, framework `platformio`, features `tft,touch,lvgl,wifi,rgb`, `output_dir` = project root).
  - Rewrite `platformio.ini`:
    - `[platformio] default_envs = cyd`, so a plain `pio run` / `pio run -t upload` targets the CYD;
    - a common `[env]` with `lib_deps = bblanchon/ArduinoJson@^7`;
    - `[esp32_base]` with platform `platformio/espressif32@6.12.0`, `board = esp32dev`, `board_build.partitions = huge_app.csv`, LVGL pinned to the exact resolved version, TFT_eSPI, XPT2046, `extra_scripts = pre:tools/gen_web.py` (an empty header until Task 9), and `monitor_speed = 115200`;
    - `cyd` and `esp32-38pin` with `extends = esp32_base` plus each env's pin flags;
    - `native` with `platform = native`, `test_framework = unity`, `build_flags = -std=gnu++17` and no `extra_scripts`.
  - Replace the generator's `ledcAttach` and `lv_tft_espi_create` as described above.
  - Copy `/tmp/claude-1000/-home-marcelo-dev-embedded-esp32-cyd/e56e8ba2-3865-4f8a-9f87-9880ac19b230/scratchpad/*.md` into `docs/port-notes/`, and this plan into `docs/superpowers/plans/`.
  - The orchestrator writes `include/secrets.h`.
- [ ] **Step 2: Smoke test.** `test_smoke` asserts `TEST_ASSERT_EQUAL(1, 1)`. Run `pio test -e native`; expected PASS.
- [ ] **Step 3: Color test screen.** `main.cpp` shows four bars, red/green/blue/white, labelled "RED/GREEN/BLUE/WHITE", on a black background.
- [ ] **Step 4: Build and record the baseline.** Run `pio run -e cyd && pio run -e esp32-38pin`; expected SUCCESS. Record RAM and Flash in `docs/budget.md`. If static RAM already exceeds 110 KB, report it to the orchestrator before continuing.
- [ ] **Step 5: Erase, flash and confirm colors.**
  - Run `pio run -e cyd -t erase --upload-port /dev/ttyUSB0`; expected "Chip erase completed successfully".
  - Then run `pio run -e cyd -t upload --upload-port /dev/ttyUSB0`.
  - **Ask the user** to confirm the colors match the labels and the background is black. If they don't, flip the inversion default in `board.h` and repeat.
- [ ] **Step 6 (Haiku): Commit and push.** Commit `chore: scaffold LVGL project for CYD and 38-pin boards`, then run `git remote add origin git@github.com:mrgadotti/esp32-ham-clock-lvgl.git && git branch -M main && git push -u origin main` (SSH; if `ssh -T git@github.com` fails, stop and ask the user). Expected: `git ls-remote origin main` prints the local HEAD hash.

### Task 1b (Haiku, parallel with Task 2): LICENSE, NOTICE and initial README

**Files:**
- Create: `LICENSE`, `NOTICE`, `README.md`

- [ ] **Step 1: LICENSE.** Use the official GPLv3 text from `https://www.gnu.org/licenses/gpl-3.0.txt`, unmodified. Check: `head -3 LICENSE` shows "GNU GENERAL PUBLIC LICENSE" and "Version 3, 29 June 2007".
- [ ] **Step 2: NOTICE.**
  - The derivation statement: derived from ESP32-HAM-CLOCK, © 2026 Konrad Wisniewski SP3KON, MIT License, with the full MIT text copied from `../ESP32-HAM-CLOCK/LICENSE`.
  - Third-party licenses: LVGL (MIT), TFT_eSPI (FreeBSD/MIT), XPT2046_Touchscreen (MIT), ArduinoJson (MIT) and the Montserrat font (OFL 1.1).
- [ ] **Step 3: README.md (English).** Sections:
  - Features.
  - Hardware, with a pin table for `cyd` and `esp32-38pin`.
  - Requirements (VS Code + PlatformIO, or `pip install platformio`).
  - Build (`pio run -e cyd`, `pio run -e esp32-38pin`) and Tests (`pio test -e native`).
  - Flashing over USB:
    - first time: `pio run -e cyd -t erase && pio run -e cyd -t upload`;
    - then: `pio run -e cyd -t upload`;
    - monitor: `pio device monitor -b 115200`.
  - Flashing from the browser (the GitHub Pages link).
  - First run:
    - AP `ESP32-HAM-CLOCK` / `1234567890` at `http://192.168.4.1`;
    - touch calibration, and how to force it by holding the screen at boot.
  - Configuration: what lives on the TFT Settings screen and what lives in the web page; `include/secrets.h.example` for development.
  - Color inversion.
  - Credits and License (GPLv3 + NOTICE).
  - Sections about later tasks get the final text this plan defines; Task 21 only checks numbers and links.
- [ ] **Step 4: Commit and push.** Commit `docs: add GPLv3 license, notice and README` and push.

### Task 22 (Sonnet, new agent, parallel after Task 1): GitHub Actions and a GitHub Pages web flasher

**Files:**
- Create: `.github/workflows/build.yml`, `.github/workflows/pages.yml`, `flasher/index.html`, `flasher/manifest-cyd.json`, `flasher/manifest-esp32-38pin.json`, `tools/merge_bin.py`

**Interfaces — Consumes:** the `cyd`, `esp32-38pin` and `native` envs (Task 1).

- [ ] **Step 1: `build.yml`.**
  - Triggers: `push` and `pull_request`.
  - Setup: ubuntu-latest, `actions/checkout@v4`, `actions/setup-python@v5` (3.11), `pip install platformio`.
  - Cache `~/.platformio` and `.pio`, keyed on `hashFiles('platformio.ini')`.
  - Job `test`: `pio test -e native`.
  - Job `build`: matrix `env: [cyd, esp32-38pin]`, runs `pio run -e ${{ matrix.env }}` and uploads `.pio/build/<env>/firmware.bin` as an artifact.
- [ ] **Step 2: `tools/merge_bin.py <env>`.**
  - Runs `esptool.py --chip esp32 merge_bin -o dist/<env>/firmware-merged.bin --flash_mode dio --flash_size 4MB 0x1000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware.bin`.
  - Paths come from `.pio/build/<env>/` and `~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin`.
- [ ] **Step 3: Flasher page and manifests.**
  - `flasher/index.html` (English) is a static page that loads ESP Web Tools with `<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module">`.
  - One `<esp-web-install-button manifest="manifest-<env>.json">` per board: "CYD ESP32-2432S028R" and "ESP32 38-pin + external ILI9341".
  - It states that Chrome or Edge is required (Web Serial) and gives the README's first-run instructions.
  - Each manifest is `{"name":"ESP32-HAM-CLOCK-LVGL","version":"<tag or sha>","new_install_prompt_erase":true,"builds":[{"chipFamily":"ESP32","parts":[{"path":"firmware-<env>.bin","offset":0}]}]}`.
- [ ] **Step 4: `pages.yml`.**
  - Triggers: push to `main` and tags `v*`.
  - Build both envs and run `merge_bin.py`.
  - Assemble `_site/`: `index.html`, the manifests with `version` filled in by `sed`, `firmware-cyd.bin` and `firmware-esp32-38pin.bin`.
  - Deploy with `actions/upload-pages-artifact@v3` + `actions/deploy-pages@v4`, `permissions: pages: write, id-token: write`, `concurrency: pages`.
  - On a `v*` tag, also attach the `.bin` files to a GitHub Release (`softprops/action-gh-release@v2`).
- [ ] **Step 5 (Haiku): Validate locally, then push.**
  - `python tools/merge_bin.py cyd` produces `dist/cyd/firmware-merged.bin`.
  - `esptool.py image_info` on the firmware reports no error.
  - `actionlint`, if available, reports no errors.
  - Commit `ci: build workflow and GitHub Pages web flasher` and push. Watch with `gh run watch` until both workflows are green.
- [ ] **Step 6 (manual step for the user, requested by the orchestrator).**
  - GitHub → Settings → Pages → Source = "GitHub Actions".
  - Open `https://mrgadotti.github.io/esp32-ham-clock-lvgl/` in Chrome and flash the CYD with erase.
  - Check that it boots.

### Task 2: Touch HAL and 3-point affine calibration

**Files:**
- Create: `lib/core/touch_cal.{h,cpp}`, `src/hal/touch.{h,cpp}`, `src/hal/touch_cal_ui.{h,cpp}`, `test/test_touch_cal/test_main.cpp`

**Interfaces — Produces:**
```cpp
struct TouchCal { float a, b, c, d, e, f; };   // x = a*rx + b*ry + c ; y = d*rx + e*ry + f
bool touchCalSolve(const int16_t raw[3][2], const int16_t scr[3][2], TouchCal& out); // false if collinear
void touchCalApply(const TouchCal&, int16_t rx, int16_t ry, int16_t& x, int16_t& y); // clamp 0..319 / 0..239
TouchCal touchCalDefault();                    // equivalent of the original map(200..3800)
void touchInit(); bool touchReadRaw(int16_t& rx, int16_t& ry); lv_indev_t* touchCreateIndev();
void touchSetCal(const TouchCal&); bool touchHeldAtBoot(uint32_t ms);
void touchSetMirror(bool mirror180);           // x=319-x, y=239-y after calibration
void touchSetGestureSink(void (*sink)(Gesture));   // wired to UiManager in Task 7
void touchCalRun(void (*onDone)(bool saved));  // calibration UI using plain lv_* + built-in font
```
- **Read callback:** on press it stores the origin; on release it calls `classifyGesture(dx,dy)`. If the result is not `None`, it calls the sink and reports the touch as released without a click.
- **`classifyGesture`** comes from Task 4. If Task 4 is not merged yet, use a local stub that returns `None`; the wave B merge resolves it.

- [ ] **Step 1: Write the failing tests.**
  - `test_cal_identity_roundtrip`: build `scr` from a known rotation + scale transform of `raw`, solve, and apply. Expected error < 0.5 px on the 3 points and on a 4th point.
  - `test_cal_rejects_collinear`: three collinear points → `false`.
  - `test_cal_clamps`: out-of-range raw values clamp to 0..319 / 0..239.
- [ ] **Step 2: Run them to see them fail.** `pio test -e native -f test_touch_cal` → FAIL (undefined).
- [ ] **Step 3: Implement the solver and driver.**
  - The solver uses Cramer's rule on 3×3.
  - `touch.cpp` uses `SPIClass(VSPI)` with the `board.h` pins, `XPT2046_Touchscreen` with IRQ, a 4-sample average, and minimum pressure 400.
- [ ] **Step 4: Run the tests.** Expected PASS.
- [ ] **Step 5: Calibration UI.** `touchCalRun` is a function, not a `Screen`, because `Screen`, fonts and theme arrive in Tasks 3 and 7.
  - It builds its own LVGL screen with the built-in Montserrat 14.
  - Targets at (32,24), (288,120) and (160,216), then a verify target at (160,120). An error > 8 px shows a message and restarts the calibration.
  - It saves NVS `tcal` with `Preferences` directly, namespace `hamclk`. Task 5 moves this into `ConfigStore`.
  - At boot, if there is no `tcal` **or** `touchHeldAtBoot(2000)` is true, it runs `touchCalRun`.
- [ ] **Step 6: Check on hardware.** Upload, calibrate, and tap test markers in all 4 corners; the dot must land under the finger (±5 px). Reboot without touching; it must not recalibrate.
- [ ] **Step 7: Commit.** `feat: touch driver with 3-point affine calibration`.

### Task 3: Fonts, i18n, theme and header widget

**Files:**
- Create: `tools/gen_fonts.sh`, `tools/fonts/` (TTF + OFL), `src/ui/fonts/*.c`, `lib/core/i18n.{h,cpp}`, `src/ui/theme.{h,cpp}`, `src/ui/widgets/header.{h,cpp}`, `test/test_i18n/test_main.cpp`

**Interfaces — Produces:**
- `LV_FONT_DECLARE(font_14|font_18|font_24|font_clock_48)`
- `enum class Lang : uint8_t { EN = 0, PT_BR = 1 }`
- `enum class Tr : uint16_t { ..., Count }` and `const char* tr(Tr)`
- `void i18nSetLang(Lang)`
- `Lang langFromCode(const char*)`: `"en"`, `"pt-br"` and `"pt_br"`; anything unknown → EN.
- `void themeApply(lv_display_t*)`: background `0x000000`, text `0xFFFFFF`, accent orange `0xFF8C00`, frequency yellow `0xFFFF00`, mode green `0x00FF00`, error red `0xFF0000`.
- `lv_obj_t* headerCreate(lv_obj_t* parent, Tr title)`: 26 px high, title on the left, free text on the right.
- `void headerSetClock(lv_obj_t*, const char*)`: each screen picks "HH:MMZ" in UTC or local time (Weather, Forecast and APRS show local time, as the original does).

- [ ] **Step 1: Write the failing tests.**
  - `test_tr_default_en`: `tr(Tr::Weather)` == `"WEATHER"`.
  - `test_tr_pt`: after `i18nSetLang(Lang::PT_BR)`, the same key returns `"CLIMA"`.
  - `test_lang_from_code`: `"pt-br"` and `"pt_br"` → PT_BR, `"xx"` → EN.
  - `test_every_key_has_both_langs`: loop over `0..Count`; no `nullptr` or empty string.
- [ ] **Step 2: Run them to see them fail.**
- [ ] **Step 3: Fill the string table.** Include **every** visible string of every screen, including the column headers and menu titles the original left hard-coded in English (see `docs/port-notes/screens_A.md` i18n section and `screens_B.md`).
- [ ] **Step 4: Run the tests.** Expected PASS.
- [ ] **Step 5: Generate the fonts.**
  - `tools/gen_fonts.sh` runs `npx lv_font_conv --no-compress --font tools/fonts/Montserrat-Medium.ttf --bpp 4 --format lvgl --range 0x20-0x7F,0xA0-0xFF --size N` for 14, 18 and 24, and `--size 48 --symbols "0123456789:-"` for the clock.
  - Each generated font gets `.fallback = &lv_font_montserrat_14`.
  - In `lv_conf.h`, `LV_FONT_MONTSERRAT_14` is the only built-in left enabled.
- [ ] **Step 6: Check on hardware.** A test screen shows "Previsão Pressão Umidade ÁÉÍÓÚ ãõç 14:05" in each font. Upload; the user confirms the accents are legible.
- [ ] **Step 7: Commit.** `feat: Latin-1 fonts, i18n EN/PT-BR and dark theme`.

### Task 4: Pure core — types, geo, bands/modes, ring buffer, backoff, gesture

**Files:**
- Create: `lib/core/{types.h,geo,band_mode,ring_buffer.h,backoff,gesture}`, `test/test_geo`, `test/test_band_mode`, `test/test_ring_buffer`, `test/test_backoff`, `test/test_gesture`

**Interfaces — Produces:** `types.h` with **every** POD struct listed under "Core interfaces"; later tasks fill them in but never redefine them. Plus:
```cpp
bool latLonToLocator(double lat, double lon, char out[7]);
bool locatorToLatLon(const char* loc, double& lat, double& lon);    // 4 or 6 chars, square centre
float distanceKm(double, double, double, double); float bearingDeg(double, double, double, double);
enum Band : uint8_t { B160, B80, B60, B40, B30, B20, B17, B15, B12, B10, B6, VHF, UHF, SHF, BAND_UNKNOWN };
Band bandFromKhz(uint32_t khz); const char* bandName(Band);
enum Mode : uint8_t { MODE_CW, MODE_SSB, MODE_DIGI, MODE_UNKNOWN }; Mode modeFrom(const char* comment, uint32_t khz);
bool passesFilter(Band, Mode, uint16_t bandMask, uint8_t modeMask);  // mask 0 = ALL
uint32_t backoffMs(uint8_t failures);                                  // 5 s, 10, 20, 40, 80 … capped at 300 s
enum class Gesture : uint8_t { None, SwipeLeft, SwipeRight }; Gesture classifyGesture(int dx, int dy);
ScreenId nextScreen(const uint8_t order[12], uint16_t enabledMask, ScreenId cur, int dir); // Settings always last
```
Masks:
- **Band:** bit = `Band` value (`1u << B40`).
  - DX and HamAlert tiles: ALL + `{160m,80m,40m,30m,20m,17m,15m,12m,10m,VHF,UHF,SHF}`.
  - POTA tiles: ALL + `{160m,80m,40m,30m,20m,17m,15m,12m,10m}`.
  - B60 and B6 have no tile and pass only when the mask is 0.
- **Mode:** CW=1, SSB=2, DIGI=4.
- `nextScreen` lives in `gesture.cpp` with a copy of the `ScreenId` enum values as `uint8_t`, so `lib/core` doesn't depend on LVGL.

- [ ] **Step 1: Write the failing tests (exact values).**
  - Geo:
    - `latLonToLocator(-23.55,-46.63)` == `"GG66qk"`;
    - `locatorToLatLon("GG66qk")` is within 0.05° of (-23.55,-46.63);
    - `distanceKm(0,0,0,1)` = 111.19 ± 0.1;
    - `bearingDeg(0,0,1,0)` = 0 ± 0.1 and `bearingDeg(0,0,0,1)` = 90 ± 0.1.
  - Bands:
    - `bandFromKhz`: `7074`=B40, `10136`=B30, `5357`=B60, `50313`=B6, `144300`=VHF, `432100`=UHF, `10368000`=SHF, `500`=BAND_UNKNOWN.
  - Modes:
    - `modeFrom("FT8 -12dB",14074)`=DIGI, `("CW 599",14025)`=CW;
    - `("",14250)`=SSB, `("",14025)`=CW (CW sub-band by frequency).
  - Filters:
    - `passesFilter(B30, MODE_CW, 1u<<B30, 0)`=true, `(B20, MODE_CW, 1u<<B30, 0)`=false;
    - `(B60, MODE_SSB, 1u<<B40, 0)`=false, `(B60, MODE_SSB, 0, 0)`=true;
    - `(B20, MODE_DIGI, 0, 1)`=false.
  - Ring buffer: pushing 3 into capacity 2 → size 2, `at(0)` is the newest; `moveToFrontIf` dedupes.
  - Backoff: `backoffMs(0)`=5000, `backoffMs(10)`=300000.
  - Gesture: `classifyGesture(-60,10)`=SwipeLeft, `(60,10)`=SwipeRight, `(30,5)`=None, `(-60,50)`=None.
  - Screen order (`test_next_skips_disabled_and_wraps`): order `{1,2,3,0,…}` with enabled `{1,3}`:
    - next(1)=3, next(3)=Settings, next(Settings)=1;
    - prev(1)=Settings.
- [ ] **Step 2: Run them to see them fail.**
- [ ] **Step 3: Implement.** Port `latLonToMaidenhead6` (L4205), `locatorToLatLon`, `calculateDistance`/`calculateBearing`, `getBand` (L13357) and `getMode`, using kHz everywhere.
- [ ] **Step 4: Run the tests.** `pio test -e native` → PASS.
- [ ] **Step 5: Commit.** `feat: pure types, geo, band/mode, ring buffer, backoff and gesture helpers`.

### Task 5: Configuration — struct, JSON codec, NVS store

**Files:**
- Create: `lib/core/config.h`, `lib/core/config_codec.{h,cpp}`, `src/app/config_store.{h,cpp}`, `test/test_config_codec/test_main.cpp`
- Modify: `src/hal/touch_cal_ui.cpp` (use `ConfigStore` for `tcal`)

**Interfaces — Produces:**
- `struct Config` has one field for **every** key in `docs/port-notes/config_web.md` §1 that stays in use, plus `tcal`, `aprs_sort`, `scr_en` and `ap_pass`.
- Excluded keys:
  - `menu_hue`, `tft_style`, `tft_table_size`, `openwebrx_url`;
  - `pota_host`, `pota_port`, `pota_filter`;
  - `touch_xmin/xmax/ymin/ymax/swap/invx/invy/rot`.
- One table, `const ConfigField kFields[]` = `{jsonKey, nvsKey, type, offset, size, min, max, isSecret, needsRestart}`, drives both the codec and the store, so names live in one place.
- `needsRestart` is true for `wifi_*`, `ap_pass` and `uart2_*`.
```cpp
void configDefaults(Config&);
bool configToJson(const Config&, char* buf, size_t len);              // secrets → "<key>_set": bool
ApplyResult configApplyJson(Config&, const char* json);               // partial; clamp; {ok, needsRestart, error[64]}
void configStoreLoad(Config&); void configStoreSave(const Config&);   // Preferences "hamclk"
void cfgGet(Config& out);                                             // any task: copy under mutex
const Config& cfg();                                                  // UI task only
void cfgUpdate(void (*fn)(Config&));                                  // mutate + save + postUiEvent(ConfigChanged)
```
- [ ] **Step 1: Write the failing tests.**
  - `test_defaults_match_original`:
    - cluster `dxspots.com:7300`, APRS `rotate.aprs2.net:14580`, radius 50, beacon 29 min;
    - lang EN, backlight 100, `tft_sw_sec` 30;
    - APRS passcode `"0"` (not an octal literal), `scr_en` = all 12 bits.
  - `test_get_masks_secrets`: the JSON has no `wifi_pass` key and contains `"wifi_pass_set":true`.
  - `test_partial_update_keeps_absent`: applying `{"user_callsign":"PY2XX"}` leaves `wifi_ssid` unchanged.
  - `test_empty_secret_keeps`: `{"wifi_pass":""}` keeps the current password.
  - `test_clamps`: `aprs_radius:0` → 1, `tft_bl:0` → 5, `tft_sw_sec:99999` → 3600.
  - `test_rejects_bad_json`: returns an error and the Config is unchanged.
  - `test_needs_restart`: true for a `wifi_ssid` or `uart2_baud` change, false for a `user_callsign` change.
- [ ] **Step 2: Run them to see them fail.**
- [ ] **Step 3: Implement** with ArduinoJson 7.
- [ ] **Step 4: Run the tests.** Expected PASS.
- [ ] **Step 5: Check on hardware.** Boot logs `[CFG] callsign=<...> ssid=<...>` with the defaults (the flash was erased) and `ssid=NASA` from `secrets.h`.
- [ ] **Step 6: Commit.** `feat: table-driven config with NVS store and JSON codec`.

### Task 6: Runtime skeleton — tasks, DataStore, events, WiFi, NTP, memory log

**Files:**
- Create: `src/app/{data_store.h,events.{h,cpp},mem_log.{h,cpp},service_coords.{h,cpp}}`, `src/net/{wifi_manager,time_service}.{h,cpp}`
- Modify: `src/main.cpp`

**Interfaces — Produces:**
- `DataStore g_store`, `postUiEvent` and `popUiEvent` (queue depth 4).
- `void wifiTick(uint32_t now)`:
  - tries SSID1, then SSID2, then the AP fallback;
  - STA retries follow `backoffMs`; while in AP mode it retries STA every 2 min;
  - it turns the AP off once STA connects, and writes `g_store.net`.
- `bool timeValid(); void timeTick(uint32_t now)`: NTP `pool.ntp.org` every hour, skipped while GPS time is valid.
- `void localTimeOf(time_t utc, struct tm&)`: float-hours `timezone` offset, as in the original.
- `bool serviceCoords(double& lat, double& lon)`: the GPS branch is stubbed `false` until Task 17.
- `void memLogTick(uint32_t now)`: prints `[MEM] free=%u minfree=%u largest=%u lvfree=%u` every 10 s (`lvfree` comes from `lv_mem_monitor`, read by the UI task and published to an atomic).

- [ ] **Step 1: Wire `main.cpp`.**
  - Call `configStoreLoad`, `displayInit` and `touchInit`.
  - Create the `ui`, `net` and `uart` tasks with the priorities, cores and stacks in Global Constraints; the `uart` task is empty for now.
  - `loop()` calls `vTaskDelete(NULL)`.
- [ ] **Step 2: Check on hardware.**
  - Upload and run `pio device monitor -b 115200`.
  - Expected: it joins `NASA` over DHCP, prints the IP, has a valid UTC time within 30 s, and `[MEM]` stays stable for 5 min.
  - Turn the test AP off for 1 min. Expected: the `ESP32-HAM-CLOCK` AP comes up, then it returns to STA when the network is back.
- [ ] **Step 3: Commit.** `feat: FreeRTOS runtime with data store, events, wifi and time`.

### Task 7: UiManager — carousel, swipe, auto-switch, overlay host, Clock and boot screens

**Files:**
- Create: `src/ui/{ui_manager.{h,cpp},screen.h}`, `src/ui/screens/{registry.cpp,boot_screen,clock_screen}.{h,cpp}`, `src/ui/overlays/alert_overlay.{h,cpp}` (the host only; content comes in Tasks 14, 16 and 19)

**Interfaces — Consumes:** `nextScreen`, `touchSetGestureSink`, `cfg()`, `popUiEvent`, `headerCreate`.

**Interfaces — Produces:**
- `void uiInit()` and `void uiTask(void*)`.
- `void uiRequest(ScreenId)`, `void uiNext()`, `void uiPrev()`: they only set a pending target, which `UiManager::tick` applies by deleting the old screen and creating the new one.
- `void uiOpenModal(Screen*)` and `void uiCloseModal()`.
- Auto-switch (`tft_autosw`, `tft_sw_sec`) pauses on Settings, in a modal, while an overlay is up, and for 10 s after any touch.
- `void overlayShow(const UiEvent&)`:
  - a full-screen object on `lv_layer_top` with `LV_OBJ_FLAG_CLICKABLE`, so it swallows touches;
  - a close X button;
  - timeouts: APRS and weather use `cfg().alrt_scr_s` (1–60 s), beaconing is fixed at 30 s.
- On `EventKind::StartTouchCal`, it calls `touchCalRun`.

- [ ] **Step 1: Boot screen.** "ESP32-HAM-CLOCK" in font_24, the callsign, and boot log lines in font_14. It shows for 3 s or until WiFi connects.
- [ ] **Step 2: Clock screen.** Port of `drawHamClock` (from L4720; see the HAM_CLOCK section of `screens_A.md`).
  - Callsign (font_24) and "OPERATOR STATION".
  - Clock `HH:MM:SS` in `font_clock_48`, showing `--:--:--` while the time is invalid. UTC or local per `screen1_time`.
  - UTC/Local label, and the full date in EN/PT.
  - The IP, or "NO CONNECTION" in red.
  - Green/red status dot, plus a "GPS" badge while there is a fix.
  - Tapping the UTC/Local label toggles it and saves `screen1_time`.
- [ ] **Step 3: Check on hardware.**
  - Swiping left and right between Clock and the Settings placeholder works.
  - A vertical drag never switches screen.
  - Auto-switch fires every `tft_sw_sec`.
  - 50 fast swipes cause no crash or leak (`lvfree` returns to its starting value ± 1 KB).
- [ ] **Step 4: Commit.** `feat: UI manager with swipe carousel, clock and boot screens`.

### Task 8: Settings screen

**Files:**
- Create: `src/ui/screens/settings_screen.{h,cpp}`

**Interfaces — Consumes:** `cfgUpdate`, `displaySet*`, `touchSetMirror`, `i18nSetLang`, `touchCalRun`, `g_store.net`.

Content is a vertical scroll list in font_14:
- Brightness slider 5–100: applied live, saved on release.
- Night dimming: switch + slider 0–90 % (`tft_ndim`).
- "Calibrate touch" button.
- "Invert colors" switch (`tft_inv`).
- Rotation dropdown 0°/180° (`tft_rot` 1/3, which also calls `touchSetMirror`).
- Language EN/PT-BR (`tft_lang`).
- Auto-switch dropdown: Off/10/15/30/60/120/300 s.
- Enabled-screen checkboxes (`scr_en`) for the **12** carousel screens; Game and Settings are not listed.
- Read-only labels: SSID, IP, RSSI, free heap and firmware version.

- [ ] **Step 1: Check on hardware.**
  - Every control takes effect immediately and persists across a reboot.
  - Disabling DX removes it from the carousel.
  - Rotation 180° keeps touch aligned with no recalibration.
- [ ] **Step 2: Commit.** `feat: on-device settings screen`.

### Task 9: Config-only web page

**Files:**
- Create: `web/config.html`, `src/net/web_config.{h,cpp}`
- Modify: `tools/gen_web.py` (real gzip of `web/config.html` into `src/net/config_html.h`)

**Interfaces — Consumes:** `configToJson`, `configApplyJson`, `cfgUpdate`, `g_store.gps`, and the bridge state (from Task 17; until then the bridge routes return `{"enabled":false}`).

**Routes:**
- Kept:
  - `GET /`: gzip, with `Content-Encoding: gzip`.
  - `GET /api/config`.
  - `POST /api/save`:
    - success is 200 `{"status":"ok","restart":bool}`, and it restarts 1.5 s later only if `needsRestart`;
    - failure is 400 `{"error":...}`, with nothing changed.
  - `POST /api/reset_touch`: deletes `tcal` and posts `StartTouchCal`.
  - `GET /api/status`: ip, rssi, uptime, heap, version, gps, `qrz_status` (from Task 11; `"disabled"` until then), and the read-only `uart2_rx_pin`/`uart2_tx_pin`.
  - `GET /api/uart_status`, `GET /api/bridge_bt_status`, `POST /api/bridge_bt_scan`, `POST /api/bridge_bt_pair`.
- Removed:
  - `/api/spots`, `/api/aprs`, `/api/pota`, `/api/hamalert`, `/api/propagation`;
  - `/api/reset_tft_rotation`;
  - the CSS files and manuals.

**Page:**
- Vanilla JS, no external libraries, under 40 KB uncompressed. English source with an EN/PT-BR string table in JS, default EN, the choice remembered in `localStorage`.
- Sections follow the groups in `config_web.md` §1:
  - WiFi + AP password;
  - Station/location/timezone;
  - DX Cluster, POTA, HamAlert;
  - APRS-IS, APRS beacon, APRS alerts;
  - OpenWeather, weather alerts;
  - LED, QRZ;
  - UART2/GPS/TNC/BT bridge, with scan/pair;
  - Display (including the 12 screen-order selects).
- Password fields are empty, with a "••• (set)" placeholder when `_set` is true.
- JSON backup export/import (the export omits secrets).
- Mobile layout at 360 px.

- [ ] **Step 1: Check on hardware.**
  - `curl -s http://<ip>/api/config | grep -o '"[a-z_]*pass[a-z_]*"'` → only `*_set` keys.
  - `curl -X POST -d '{"user_callsign":"PY2TEST"}' http://<ip>/api/save` → the callsign changes on the Clock screen, WiFi is intact, and there is no reboot.
  - `curl -X POST -d '{bad' http://<ip>/api/save` → 400.
  - Open the page on a phone and save a field.
- [ ] **Step 2: AP mode.** With an invalid SSID, join `ESP32-HAM-CLOCK`, open `http://192.168.4.1` and set `NASA`. Expected: it restarts and connects.
- [ ] **Step 3: Commit.** `feat: config-only web portal with masked secrets and partial save`.

### Task 10: DX Cluster — parser, telnet service, table widget, DX screen

**Files:**
- Create: `lib/core/dx_parse.{h,cpp}`, `src/svc/dx_cluster.{h,cpp}`, `src/ui/widgets/spot_table.{h,cpp}`, `src/ui/widgets/filter_menu.{h,cpp}`, `src/ui/screens/dx_screen.{h,cpp}`, `test/test_dx_parse/test_main.cpp`

**Interfaces — Produces:**
- `bool parseDxLine(const char* line, uint32_t nowUtc, Spot& out)`: sanitizes the comment and extracts a locator from it.
- `void dxTick(uint32_t now)`, ported from `connectToCluster`/`sendClusterConfigCommands`/`handleTelnetData`:
  - login, `set/qra`, the configured filters;
  - 30 s keepalive and 5 min silence watchdog;
  - reconnect with `backoffMs`;
  - reads complete lines from a `char[256]` buffer and inserts **all** of them;
  - computes distance with `serviceCoords`.
- `lv_obj_t* spotTableCreate(lv_obj_t* parent, const SpotTableCols& cols)`, and:
  ```cpp
  void spotTableSet(lv_obj_t* table, size_t n, bool (*rowAt)(size_t i, Spot& out, void* ctx), void* ctx);
  ```
  - a single `lv_table`, with the row count set to `n`;
  - `rowAt` copies one row under the store lock;
  - column colors in `LV_EVENT_DRAW_TASK_ADDED`: frequency yellow, mode green, call white, message/country orange;
  - the scroll position is kept.
- `void filterMenuOpen(lv_obj_t* parent, const uint8_t* bands, uint8_t nBands, uint16_t* bandMask, uint8_t* modeMask, void (*onSave)())`: OK saves, CANCEL discards.

- [ ] **Step 1: Write the failing tests.**
  - `test_parse_basic`: `"DX de PY2XX:     14025.0  K1ABC        CW 599                         1234Z"` gives:
    - call `K1ABC`, khz 14025, spotter `PY2XX`, comment `CW 599`;
    - band B20, mode CW, time 12:34 UTC of the `nowUtc` day.
  - `test_parse_locator_in_comment`: a comment containing `FN31` → `loc` = `FN31`.
  - `test_dx_rejects_garbage`: `""`, `"DX de"`, `"hello"`, and a line with frequency `abc` all return false.
  - `test_long_comment_truncated_and_sanitized`: a 200-char comment with bytes `0x01` and `0xC3` → it ends in `\0`, has length ≤ 39, and is printable ASCII only.
- [ ] **Step 2: Run them to see them fail.**
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run the tests.** Expected PASS.
- [ ] **Step 5: DX screen.**
  - Header "DX CLUSTER".
  - A 40×26 filter button at the top-left opens `filterMenuOpen` with ALL/CW/SSB/DIGI and the 12 DX bands, saved to `s2_mode`/`s2_band`.
  - The table is filtered with `passesFilter`.
  - Shows "Waiting for spots" when empty.
  - Refreshes only when `g_store.dx.version()` changes.
- [ ] **Step 6: Check on hardware.**
  - Spots arrive within 2 min.
  - Dragging scrolls through all 100.
  - The 40m filter shows only 40m and persists across a reboot.
  - `lvfree` stays ≥ 8 KB with 100 rows.
- [ ] **Step 7: Commit.** `feat: DX cluster service and scrollable DX screen`.

### Task 11: QRZ — queue, cache and enrichment

**Files:**
- Create: `lib/core/qrz_cache.{h,cpp}`, `src/svc/qrz.{h,cpp}`, `src/net/http_fetch.{h,cpp}`, `test/test_qrz/test_main.cpp`

**Interfaces — Produces:**
- `class QrzCache`: 20 entries, 15 min TTL, `bool get(const char* call, uint32_t now, QrzInfo&)`, `void put(const char* call, const QrzInfo&, uint32_t now)`.
- `class QrzQueue`: 20 entries, deduped, 2 attempts, 3 s interval.
- `bool qrzParseXml(const char* xml, QrzInfo&)`.
- `bool httpGet(const char* url, char* buf, size_t len, uint32_t timeoutMs)` and `bool httpGetJson(const char* url, JsonDocument& doc, const JsonDocument* filter)`: `WiFiClientSecure::setInsecure()`, 8 s timeout, streaming.
- `void qrzTick(uint32_t now)`: updates country/locator/lat/lon in `dx` and `pota`.
- `const char* qrzStatusStr()`: `"disabled"`, `"ok"` or `"error: ..."`.

- [ ] **Step 1: Write the failing tests.**
  - `test_cache_ttl`: an entry expires after 15 min.
  - `test_queue_dedupe`.
  - `test_parse_xml`: a fixture with `<grid>FN31pr</grid><country>United States</country><lat>41.7</lat><lon>-72.7</lon>` gives those values.
  - `test_parse_session_error`: an `<Error>` response → false.
- [ ] **Step 2: Run them to see them fail.**
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run the tests.** Expected PASS.
- [ ] **Step 5: Check on hardware.** With the user's QRZ credentials, DX rows gain a country. Without credentials, the log shows `[QRZ] disabled` and `/api/status` reports `"qrz_status":"disabled"`.
- [ ] **Step 6: Commit.** `feat: QRZ lookup queue and cache`.

### Task 12: POTA and HamAlert

**Files:**
- Create: `lib/core/{pota_parse,hamalert_parse}.{h,cpp}`, `src/svc/{pota,hamalert}.{h,cpp}`, `src/ui/screens/{pota_screen,hamalert_screen}.{h,cpp}`, `test/test_pota`, `test/test_hamalert`

**Interfaces — Produces:**
- `bool potaFromJson(JsonObjectConst, Spot&)`: uses the `api.pota.app/v1/spots` fields `activator`, `frequency` (kHz string), `mode`, `reference`, `spotTime`, `locationDesc` and `comments`.
- `bool hamalertParseLine(const char*, uint32_t nowUtc, Spot&)`: accepts a `set/json` JSON line or a DX-format line.
- POTA service: every 180 s, sorted by time descending with an index sort (no `String` copies).
- HamAlert service: every 60 s, a new connection per poll, `sh/dx 30`; it **keeps the previous list on failure**.

- [ ] **Step 1: Write the failing tests.**
  - `test_pota_fixture`: a 2-spot JSON gives 2 Spots with the correct khz, mode and call.
  - `test_pota_rejects_garbage`: an object without `activator` → false.
  - `test_hamalert_json_line`.
  - `test_hamalert_dx_line`.
  - `test_hamalert_rejects_garbage`.
- [ ] **Step 2: Run them to see them fail.**
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run the tests.** Expected PASS.
- [ ] **Step 5: Screens.**
  - Both reuse `spotTable` with the columns UTC/CALL/MHz/MODE/COUNTRY, and `filterMenu`:
    - POTA has 9 bands → `s7_*`;
    - HamAlert has 12 → `s8_*`.
  - Both get drag scroll, which the original lacked.
- [ ] **Step 6: Check on hardware.** POTA shows spots. HamAlert shows spots with the user's login, or reports itself disabled without one.
- [ ] **Step 7: Commit.** `feat: POTA and HamAlert services and screens`.

### Task 13: APRS — parsers and alert rules (pure)

**Files:**
- Create: `lib/core/{aprs_parse,aprs_wx,alert_rules}.{h,cpp}`, `test/test_aprs_parse`, `test/test_aprs_wx`, `test/test_alert_rules`

**Interfaces — Produces:**
- `bool aprsParseFrame(const char* tnc2Line, AprsStation& out)`:
  - parses `SRC>DST,PATH:info` and dispatches to uncompressed, compressed, Mic-E, object or item;
  - ported from the original L12846–13290 and L15457;
  - sanitizes the comment.
- `bool aprsDecodeWx(const char* comment, AprsWx&)`.
- `bool aprsExtractFreq(const char* text, float& mhz)`.
- `bool alertCallMatches(const char* watchToken, const char* call)`:
  - a token without SSID matches any SSID of that base call;
  - a token with SSID must match exactly;
  - a `*` suffix is a prefix match.
- `class AlertRules { void configure(const AlertCfg&); bool shouldAlert(const AprsStation&, uint32_t now); }`:
  - the watch list is tokenized once per config change;
  - alerts on a mobile SSID 7/9 within `aprs_alert_km`, and on WX stations;
  - per-call cooldown `aprs_alert_min_s`.

- [ ] **Step 1: Write the failing tests.**
  - `test_uncompressed`: `"PY2XX>APRS:!4903.50N/07201.75W-Test"` gives:
    - lat 49.058333 and lon -72.029167 (±1e-4);
    - sym `/-` and comment `Test`.
  - `test_compressed`: info `"=/5L!!<*e7>7P["` → lat 49.5, lon -72.75 (±1e-3).
  - `test_mic_e`: `"PY2XX>S32UVT:` + "`" + `(_fn\"Oj/"` gives:
    - lat 33.4273 (±1e-3, 33°25.64'N) and lon -112.129 (±1e-3, 112°07.74'W);
    - sym `/j`.
    - Why: the dest chars `S32UVT` give digits 3,3,2,5,6,4, with `U` = North, `V` = +100° longitude offset and `T` = West, and the info bytes `(` `_` `f` give 12°07.74' + 100.
    - Cross-check: also assert the result equals the original `parseAprsMicE` output, compiled once on the host with a `String` shim. If they disagree, stop and report.
  - `test_object`.
  - `test_aprs_rejects_garbage`: `"X>Y:"`, `">:"`, and a position of `9999.99N` → false.
  - `test_freq`: `"146.520MHz"` → 146.52.
  - `test_wx`: `"_220/004g005t077r000p000P000h50b09900"` → dir 220, temp 25.0 °C ±0.1, humidity 50, pressure 990.0.
  - `test_alert_wildcard`, `test_alert_base_matches_ssid`, `test_alert_cooldown`, `test_alert_mobile_ssid_distance`.
- [ ] **Step 2: Run them to see them fail.**
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run the tests.** Expected PASS.
- [ ] **Step 5: Commit.** `feat: pure APRS parsers and alert rules`.

### Task 14: APRS-IS, APRS list, APRS alert overlay, RGB LED

**Files:**
- Create: `src/svc/aprs_is.{h,cpp}`, `src/svc/aprs_ingest.{h,cpp}`, `src/hal/rgb_led.{h,cpp}`, `src/ui/screens/aprs_list_screen.{h,cpp}`
- Modify: `src/ui/overlays/alert_overlay.cpp`

**Interfaces — Produces:**
- `void aprsIngestLine(const char* tnc2, char source)`:
  - runs `aprsParseFrame`, then distance/bearing from `serviceCoords`, then `moveToFrontIf` by call;
  - runs `AlertRules`, and on a hit calls `postUiEvent(AprsAlert)` + `rgbLedBlink`;
  - used by APRS-IS (`'I'`) and by the UART2 TNC (Task 17, `'T'`/`'K'`).
- `void aprsIsTick(uint32_t now)`:
  - login `user CALL-SSID pass N vers ESP32-HAM-CLOCK-LVGL 2.0`;
  - filter `#filter r/lat/lon/radius`, re-sent after moving ≥ 1 km, at most every 30 s;
  - reads lines into `char[512]`;
  - disabled when UART2 is the APRS source.
- `void rgbLedBlink(uint8_t color, uint32_t durationMs, uint32_t blinkMs)` and `void rgbLedTick(uint32_t now)`; both are no-ops without `HAS_RGB_LED`.

- [ ] **Step 1: APRS list screen.**
  - Columns UTC/CALL/SYM/KM/FREQ, with drag scroll.
  - The title depends on the source: "APRS-IS", "APRS-TNC2-UART" or "APRS-TNC-KISS".
  - The menu (top-left button) has:
    - a TIME/CALL/DIST sort, **persisted** in `aprs_sort`;
    - checkboxes Beaconing / APRS Alert / Range Alert / LED.
  - OK saves and CANCEL discards.
- [ ] **Step 2: APRS ALERT overlay.** Port of `ALERT_Screen` (L12292; see the alert section of `screens_B.md`).
  - Pulsing frame, 500 ms.
  - Call, distance and bearing.
  - A compass drawn in a draw event (`lv_draw_line`/`lv_draw_arc`).
  - Vector car/house/person icons.
  - The comment, wrapped.
  - WX stations show the decoded values (the "WIND" label comes from `tr`).
- [ ] **Step 3: Check on hardware.** Stations appear. Add an active local call to the watch list via the web page → the overlay appears and the blue LED blinks.
- [ ] **Step 4: Commit.** `feat: APRS-IS service, APRS list screen and alert overlay`.

### Task 15: APRS radar with tap-to-zoom

**Files:**
- Create: `lib/core/radar_proj.{h,cpp}`, `src/ui/screens/aprs_radar_screen.{h,cpp}`, `test/test_radar/test_main.cpp`

**Interfaces — Produces:**
```cpp
struct RadarView { float zoom; float cxKm, cyKm; };          // view centre in km (E, N) from QTH
bool radarProject(const RadarView&, float distKm, float bearingDeg, float rangeKm,
                  int16_t radiusPx, int16_t originX, int16_t originY, int16_t& px, int16_t& py); // false if off-area
RadarView radarZoomAt(const RadarView&, int16_t tapX, int16_t tapY, float rangeKm, int16_t radiusPx,
                      int16_t originX, int16_t originY);       // ×2 (max 8), re-centred on the tap
```
- [ ] **Step 1: Write the failing tests.**
  - `test_project_east`: view {1,0,0}, 25 km at 90°, range 50, radius 100, origin (160,128) → (210,128).
  - `test_project_north`: 25 km at 0° → (160,78).
  - `test_zoom_recenters`: tapping (210,128) gives zoom 2, and the same station then projects to (160,128).
  - `test_zoom_caps_at_8`.
  - `test_outside_returns_false`.
- [ ] **Step 2: Run them to see them fail.**
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run the tests.** Expected PASS.
- [ ] **Step 5: Radar screen.** One object with `LV_EVENT_DRAW_MAIN`, no canvas.
  - Range rings at 50 % and 100 %, labelled in km, plus the **alert-distance ring** (`aprs_alert_km`).
  - North up, QTH at the centre.
  - Up to 30 stations as 5 px dots, colored mobile/fixed/WX/alert as in `getAprsRadarColorForStation` (L7299). Calls (font_14) are drawn when zoom ≥ 2 or there are ≤ 11 stations.
  - A tap calls `radarZoomAt`; a swipe is consumed by the gesture sink first.
  - A "1×" button at the bottom-right resets the view, and a "×2/×4/×8" indicator sits in the header.
- [ ] **Step 6: Check on hardware.** Tapping a station brings it to the centre, zoomed. Three taps reach ×8, and "1×" resets.
- [ ] **Step 7: Commit.** `feat: APRS radar with tap-to-zoom`.

### Task 16: APRS beacon (APRS-IS and KISS)

**Files:**
- Create: `lib/core/ax25_kiss.{h,cpp}`, `src/svc/aprs_beacon.{h,cpp}`, `test/test_ax25_kiss`
- Modify: `src/ui/overlays/alert_overlay.cpp`

**Interfaces — Produces:**
- `size_t kissEncodeUiFrame(const char* src, const char* dst, const char* const* path, uint8_t nPath, const char* info, uint8_t* out, size_t cap)`.
- `bool kissDeframe(uint8_t byte, KissRx& st)`: state in a struct, 600-byte buffer.
- `bool ax25ToTnc2(const uint8_t* frame, size_t len, char* out, size_t cap)`.
- `bool formatAprsPosition(double lat, double lon, char symTable, char symCode, const char* comment, char* out, size_t cap)`:
  - output is `!DDMM.mmN<table>DDDMM.mmW<code><comment>`;
  - minutes are **rounded** to 2 decimals, carrying at 60.00.
- `bool aprsTxCallValid(const char*)`.
- `void aprsBeaconTick(uint32_t now)`:
  - the first beacon goes out 60 s after the channel is ready, then every `aprs_interval` min (default 29);
  - every 5th beacon swaps the comment for the project URL, as the original does;
  - it sends over APRS-IS or UART2 KISS as configured;
  - it posts `UiEvent::Beaconing` and never draws directly.

- [ ] **Step 1: Write the failing tests.**
  - `test_kiss_escape`: a payload with `0xC0` and `0xDB` → `DB DC` and `DB DD`.
  - `test_kiss_roundtrip`: encode → deframe → `ax25ToTnc2` == `"PY2XX-9>APRS,WIDE1-1,WIDE2-1:!4903.50N/07201.75W-Test"`.
  - `test_format_position`:
    - (49.058333, -72.029167, '/', '-', "Test") → `"!4903.50N/07201.75W-Test"`;
    - (10.999999, 0.0, '/', '-', "") → `"!1100.00N/00000.00E-"`.
  - `test_tx_call_invalid`: `"N0CALL"` and `""` are rejected.
- [ ] **Step 2: Run them to see them fail.**
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run the tests.** Expected PASS.
- [ ] **Step 5: Beaconing overlay.** "APRS BEACONING" (spelling fixed) for 2 s, with call, position and path.
- [ ] **Step 6: Check on hardware.** Enable the beacon with a 1 min interval (temporarily, via the web page) and confirm on aprs.fi that the position arrived over APRS-IS. Restore 29 min.
- [ ] **Step 7: Commit.** `feat: APRS beacon over APRS-IS and KISS`.

### Task 17: UART2 — GPS NMEA, TNC text/KISS, BT bridge; Location screen

**Files:**
- Create: `lib/core/{nmea,bridge_proto}.{h,cpp}`, `src/svc/uart2.{h,cpp}`, `src/ui/screens/location_screen.{h,cpp}`, `test/test_nmea`, `test/test_bridge`
- Modify: `src/net/web_config.cpp` (real bridge routes), `src/app/service_coords.cpp` (enable the GPS branch)

**Interfaces — Produces:**
- `bool nmeaChecksumOk(const char*)`.
- `bool nmeaParse(const char* sentence, GpsState&)`: RMC, GGA, GSV per constellation, and the `$GPSESP32[...]` frame.
- `GpsState { bool active, fix; double lat, lon; float speedKmh; uint8_t sats, satsGps, satsGlo, satsGal, satsBds; uint32_t utc; uint32_t lastFixMs; }`.
- `BridgeEvent bridgeParseLine(const char*)` and `class BridgeFsm`, ported from L1986–2200 and L3136:
  - scan and pair;
  - BTSPP retry 3 × 2 s;
  - boot `GPS ON` at 60 s;
  - 250 ms command cooldown.
- `void uart2Task(void*)`:
  - `Serial2.setRxBufferSize(1024)` **before** `Serial2.begin(baud, cfg, PIN_UART2_RX, PIN_UART2_TX)`;
  - modes gps / aprs_tnc / aprs_tnc_kiss / esp32_bridge / none;
  - TNC2 lines and KISS frames (via `ax25ToTnc2`) go to `aprsIngestLine`;
  - a valid GPS time sets the clock with `settimeofday`.

- [ ] **Step 1: Write the failing tests.**
  - `test_rmc`: `"$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A"` gives:
    - checksum ok and fix true;
    - lat 48.1173 and lon 11.516667 (±1e-4).
  - `test_bad_checksum`: the last character changed → false.
  - `test_gga_sats`.
  - `test_nmea_rejects_garbage`.
  - `test_bridge_list`: `"BT[LIST_BEGIN]:2"`, two name/address lines, then `"BT[LIST_END]"` → 2 devices.
  - `test_bridge_btspp_retry`: with no ACK, 3 resends 2 s apart (injected clock).
- [ ] **Step 2: Run them to see them fail.**
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run the tests.** Expected PASS.
- [ ] **Step 5: Location screen.** Port of L4295.
  - GPS active/fix, satellites, LAT, LON, speed and the 6-character locator.
  - SAVE copies GPS → `user_lat/lon` only with a valid fix, and shows a "Saved" / "No fix" toast.
- [ ] **Step 6: Check on hardware.** With no module connected, the screen shows "GPS inactive" and nothing stalls. If the user has a GPS module, check fix and position.
- [ ] **Step 7: Commit.** `feat: UART2 GPS/TNC/bridge task and location screen`.

### Task 18: Propagation and HF bands

**Files:**
- Create: `lib/core/propagation_xml.{h,cpp}`, `src/svc/propagation.{h,cpp}`, `src/ui/screens/{propagation_screen,bands_screen}.{h,cpp}`, `test/test_propagation`

**Interfaces — Produces:**
- `PropagationData { char sfi[6], kindex[4], aindex[4], muf[8], updated[24]; struct { char label[10]; char day[6]; char night[6]; } band[4]; bool valid; uint32_t fetchedAt; }`.
- `bool parsePropagationXml(const char* xml, PropagationData&)`.
- The service fetches `https://www.hamqsl.com/solarxml.php` every 60 min (retry after 5 min) into a fixed 8 KB buffer.

- [ ] **Step 1: Write the failing tests.**
  - `test_prop_fixture`: minimal XML with `<solarflux>150</solarflux><kindex>3</kindex><aindex>8</aindex><muf>21.3</muf><updated>16 Nov 2026 1200 GMT</updated>` and `<band name="80m-40m" time="day">Fair</band>` → the right fields.
  - `test_prop_rejects_garbage`: an HTML error page → `valid=false`, previous data kept.
- [ ] **Step 2: Run them to see them fail.**
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run the tests.** Expected PASS.
- [ ] **Step 5: Screens.** Port of L9198 and L9380 (see sections 1–2 of `screens_B.md`).
  - SFI/K/A/MUF in font_24. K is green below 4, orange below 6, red at 6 and above.
  - Day/Night table for the 4 band groups: Good green, Fair yellow, Poor red.
  - "UPDATED UTC" line plus a data-age footer.
- [ ] **Step 6: Check on hardware.**
- [ ] **Step 7: Commit.** `feat: propagation and HF band screens`.

### Task 19: Weather, forecast, weather alerts and night dimming

**Files:**
- Create: `lib/core/{weather_json,weather_alerts}.{h,cpp}`, `src/svc/weather.{h,cpp}`, `tools/gen_icons.py`, `src/ui/icons/*.c`, `src/ui/screens/{weather_screen,forecast_screen}.{h,cpp}`, `test/test_weather`, `test/test_weather_alerts`
- Modify: `src/hal/display.cpp`, `src/ui/overlays/alert_overlay.cpp`

**Interfaces — Produces:**
- `bool weatherParseCurrent(JsonDocument&, WeatherData&)`.
- `bool weatherParseForecast(JsonDocument&, WeatherData&)`: +3h, +6h, and 2 days with the night minimum, as in the original.
- `bool weatherParseAir(JsonDocument&, WeatherData&)`.
- `class WeatherAlertScheduler { bool due(const WeatherData&, uint32_t now, const WxAlertCfg&, WeatherAlertInfo& out); }`:
  - storm: id 2xx with data ≤ 30 min old, or a storm in the forecast;
  - wind: ≥ `wx_w_thr`, or a gust ≥ thr+5;
  - the repeat interval is in **minutes**, and the label says minutes.
- `const lv_image_dsc_t* weatherIcon(int id, bool night)`: 15 dark RGB565 50×50 icons, with a vector fallback for unknown ids.
- `bool isNightFromIcon(const char* iconCode)`.
- `void displayApplyNightDim(bool night)`: target = backlight × (100 − `tft_ndim`)/100, minimum 5.

- [ ] **Step 1: Write the failing tests.**
  - `test_current_fixture`: a trimmed real OpenWeather JSON → temp, humidity, pressure, wind, id, icon and city.
  - `test_forecast_fixture`.
  - `test_air_fixture`: PM2.5 and PM10.
  - `test_weather_rejects_garbage`: `{}` → false, old data kept.
  - `test_alert_storm_now`, `test_alert_wind_gust`, `test_alert_repeat_minutes`.
- [ ] **Step 2: Run them to see them fail.**
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run the tests.** Expected PASS.
- [ ] **Step 5: Icons and service.**
  - `gen_icons.py` converts `../ESP32-HAM-CLOCK/data/icon50/*.bmp` (skipping the `w` suffix) into C arrays, and fails if the total exceeds 80 KB.
  - The service fetches current + air every 10 min (retry after 2 min), and the forecast with `cnt=24`. `lang` is en or pt_br, matching the UI language. ArduinoJson filters keep the whole JSON out of memory.
- [ ] **Step 6: Screens.** Port of L10290, L10469 and L10703.
  - **Weather:** icon, description, temperature in font_24, humidity, pressure, wind, color-coded PM2.5/PM10, the city, and a data-age footer.
  - **Forecast:** 4 columns (+3h, +6h, 2 days), each with icon, temperature, night temperature, humidity and wind, plus a data-age footer.
  - **WEATHER ALERT overlay:** a storm or wind icon.
- [ ] **Step 7: Check on hardware.** Use the user's OpenWeather key.
- [ ] **Step 8: Commit.** `feat: weather, forecast, weather alerts and night dimming`.

### Task 20: Matrix clock and the Unlis Hunter game

**Files:**
- Create: `src/ui/screens/{matrix_screen,game_screen}.{h,cpp}`

- [ ] **Step 1: Matrix screen.** Port of L10737–10930.
  - 45 rain columns. **Each column is up to 3 multi-line `lv_label`s** (dim, mid and head color), never one draw call per character, which keeps it to about 150 draw tasks per frame.
  - State in `MatrixDrop[45]`: speed 5–14, length 4–16.
  - Updated every 120 ms by an `lv_timer`.
  - Clock `HH:MM:SS` in `font_clock_48` on an opaque black box.
  - Holding the top-right corner (x ≥ 240, y ≤ 80) for 2 s opens the game with `uiOpenModal`.
- [ ] **Step 2: Game screen.** Port of L10995–11630 (see section 8 of `screens_B.md`).
  - Rings at R=115 (2m) and R=60 (70cm).
  - The Yagi pointer turns at 46°/s, +5 % per catch and +3 % after 60 s.
  - Red targets score; a green decoy costs a point; 3 misses end the game.
  - START/STOP, PTT and EXIT buttons.
  - 25 fps via a 40 ms `lv_timer`, drawn in `LV_EVENT_DRAW_MAIN` with only lines and arcs.
  - `blocksSwipe()` is true while the game runs.
- [ ] **Step 3: Check on hardware.**
  - The Matrix animation is smooth, and `lvfree` stays ≥ 8 KB while it animates.
  - The game is playable, and EXIT returns to Matrix.
- [ ] **Step 4: Commit.** `feat: matrix clock screen and hidden Unlis Hunter game`.

### Task 21: Wrap-up — budgets, soak, docs

**Files:**
- Create: `docs/hardware-checklist.md`
- Modify: `README.md`, `docs/budget.md`

- [ ] **Step 1 (Haiku): Final tests and builds.**
  - `pio test -e native` → all PASS.
  - `pio run -e cyd` and `-e esp32-38pin` → SUCCESS, flash ≤ 2.6 MB, static RAM ≤ 150 KB.
  - Record the numbers in `docs/budget.md`.
- [ ] **Step 2 (Haiku): 2-hour soak on `cyd`.**
  - All services on, auto-switch at 15 s, with the monitor log saved to `docs/soak-<date>.log`.
  - Pass criteria:
    - no reboot (no `rst:` line);
    - `largest` ≥ 45 KB at idle and `minfree` ≥ 30 KB;
    - HTTPS fetches still succeeding at the end.
- [ ] **Step 3 (Haiku): Hardware checklist.** Write `docs/hardware-checklist.md` with action → expected result for each of the 14 screens, calibration, the 3 overlays, the web page and the web flasher. The orchestrator runs it with the user and ticks each item.
- [ ] **Step 4 (Haiku): Final README update.**
  - Real RAM/flash numbers and the working flasher link.
  - A "What changed vs. ESP32-HAM-CLOCK" section.
  - Run every README command once to confirm it works as written.
- [ ] **Step 5 (Haiku): Commit and push.** Commit `docs: README, hardware checklist and budget results` and push.

---

## End-to-end verification

1. `cd esp32-ham-clock-lvgl && pio test -e native`: every `lib/core` test passes.
2. `pio run -e cyd && pio run -e esp32-38pin`: both build within budget.
3. `pio run -e cyd -t erase && pio run -e cyd -t upload --upload-port /dev/ttyUSB0 && pio device monitor -b 115200`: it joins `NASA` over DHCP and `[MEM]` is stable.
4. The Task 21 hardware checklist is run with the user on the CYD:
   - colors, calibration, swipe and scroll;
   - radar zoom;
   - Settings persistence;
   - a config-only web page.
5. The 2-hour soak passes.
6. GitHub Actions is green, and the Pages flasher flashes the board from Chrome.

## Execution waves (parallelism)

Parallel agents work in separate git worktrees (`superpowers:using-git-worktrees`), and the orchestrator merges them into `main` at the end of each wave. There is a single board, so hardware steps (upload + confirmation) are always serialized by the orchestrator.

| Wave | Runs in parallel | Needs |
|---|---|---|
| A | T1 (Sonnet), then the initial push (Haiku) | — |
| B | T1b docs (Haiku) · T22 CI/Pages (Sonnet) · T2 touch (Sonnet) · T4 pure core (Sonnet) | A |
| C | T3 fonts/i18n · T5 config · **pure parsers**: steps 1–4 of T10, T11, T12, T13, T15, T16, T17, T18, T19 (one Sonnet agent each; they touch only `lib/core/` and `test/`) | B (T4 defines `types.h`) |
| D | T6 runtime → T7 UiManager → T8 Settings ∥ T9 Web | C |
| E | Service + screen integration, two at a time: T10 ∥ T18 · T11 ∥ T12 · T14 ∥ T19 · T15 ∥ T16 · T17 ∥ T20 (T17 after T14, because it uses `aprsIngestLine`) | D, plus that task's own wave-C parsers |
| F | T21 (Haiku runs it and records results; the orchestrator walks the checklist with the user) | E |

Per task:
1. A Sonnet 5.5 implementer writes the code and tests.
2. A Sonnet 5.5 reviewer reviews it.
3. A Haiku 5.5 gate runs the full tests, both builds, the upload when there is a hardware step, and commit + push.
4. If the gate fails, the implementer gets the exact output and fixes it.

### Task 23 (user request): Whole-codebase review after all implementations

Runs after Tasks 1–22 and all follow-up fixes are merged on `main`, before Task 21's soak and the final release.

**Files:** Create `docs/review/2026-final-code-review.md`. Fixes go into the affected source files.

- [ ] **Step 1: Review the whole codebase.** Use the most capable model (`superpowers:requesting-code-review`, whole-branch scope from the first commit to `main` HEAD), not per-task diffs. Check that:
  - every Global Constraint holds: pure `lib/core`, no `String` in stores, network text sanitized, i18n EN/PT-BR with no hard-coded UI text, dark theme, budgets;
  - concurrency is safe: LVGL is touched only from the UI task, store and config mutexes are used, there is no lock-order inversion, no blocking calls hold locks;
  - memory and heap are sound: no leaks, no per-poll fragmentation, enough TLS headroom, enough LV_MEM headroom on every screen;
  - error handling and security hold up: no secrets in logs or API responses, and bounded reads on every network and UART input;
  - dead code, duplication and leftovers are gone, including Matrix and game remnants and unused hooks;
  - tests cover all pure logic, and the docs (README, NOTICE, budget) match the code.
- [ ] **Step 2: Write the report.** Record the findings in `docs/review/2026-final-code-review.md`, ranked Critical, Important, then Minor, each with `file:line`.
- [ ] **Step 3: Fix the findings.** Fix every Critical and Important finding in a single fix wave, with Sonnet implementers and a scoped re-review. Triage the Minor findings: fix the cheap ones and record the rest in the report.
- [ ] **Step 4: Verify (Haiku gate).**
  - `pio test -e native` passes.
  - `pio run -e cyd -e esp32-38pin` succeeds.
  - Flash the board and capture a 10-minute log with no panic.
  - Commit and push.
