# Open-Meteo Screens Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add two optional screens, "Weather - Open-Meteo" and "Forecast - Open-Meteo", fed by the keyless Open-Meteo API every 10 minutes, toggled in Settings and the web page like every other screen. The OpenWeather screens stay unchanged.

**Architecture:** A new pure parser in `lib/core` turns the Open-Meteo JSON into the existing `WeatherData` struct (WMO codes mapped to the OpenWeather ids that `wx_map` already turns into icons). A new net-task service `src/svc/weather_om.cpp` fills a second store slot `g_store.weatherOm`. The existing Weather and Forecast screen classes take a source parameter and read the matching slot, so no UI layout is duplicated.

**Tech Stack:** PlatformIO espressif32@6.12.0, Arduino, LVGL 9.6, ArduinoJson 7, Unity native tests (`pio test -e native`).

**Spec:** `docs/superpowers/specs/2026-11-18-open-meteo-research.md` (limits, URLs, WMO table, sample JSON). Use the cyd-dev skill for any board or LVGL question.

## Global Constraints

- Free API, no key: `https://api.open-meteo.com/v1/forecast` and `https://air-quality-api.open-meteo.com/v1/air-quality`. Limits are 600/min, 5000/h, 10000/day and are **applied per IP address**, so a home network behind NAT shares them. At one forecast call per 10 min plus one air-quality call per hour the clock uses about 170 calls/day.
- Non-commercial use only. Attribution is required (CC-BY 4.0): show "Open-Meteo.com" in the screen footer and credit Open-Meteo.com and CAMS (air quality) in the README.
- Refresh `OM_MS = 10 min`; retry after a failure `OM_RETRY_MS = 5 min` (429 bodies carry no reliable Retry-After). Air quality refreshes every 6th successful cycle (data changes hourly).
- Units: `wind_speed_unit=ms`, `timezone=auto`. Pressure is `surface_pressure` in hPa.
- Screen ids are stable numbers: add `WeatherOm = 10`, `ForecastOm = 11` (reusing the freed Matrix and Game numbers). Both are carousel screens and **off by default**: default `scr_en` stays 0x13FE and the default `screen_order` stays unchanged (it has two free slots). The `scr_en` clamp mask becomes 0x1FFE.
- Menu, header and web names: "Weather - Open-Meteo" / "Forecast - Open-Meteo" (PT: "Clima - Open-Meteo" / "Previsão - Open-Meteo").
- OpenWeather alerts (`WeatherAlertScheduler`) keep reading only `g_store.weather`. No OpenWeather code or key logic changes.
- Code, comments and commits in English. No Co-Authored-By and no "Generated with Claude Code" lines. Native tests must pass and `pio run -e cyd` must build.

## Review Focus

- Open-Meteo returns `{"error":true,"reason":"..."}` with HTTP 400/429: the parser returns false and the old data stays (test in Task 1).
- A field is null in the JSON (for example `visibility` or `uv_index` missing): parse still succeeds with defaults (Task 1).
- WMO codes outside the table (for example 100): the icon id falls back to 800 and the description key to a generic one, no crash (Task 1).
- Fewer hourly or daily entries than expected: missing columns stay `detailValid=false` (Task 1).
- Old config backups carrying `scr_en` bit 10 or 11 or order slots 10/11: they now enable the new screens; acceptable, and out-of-range values are still dropped (Task 3 test).

---

### Task 1: WMO mapping and Open-Meteo parser (core)

**Files:**
- Create: `lib/core/wx_open_meteo.h`, `lib/core/wx_open_meteo.cpp`
- Modify: `lib/core/i18n.h` (new `Tr` keys), `test/test_i18n` only if it counts keys
- Test: `test/test_wx_open_meteo/test_main.cpp`

**Interfaces:**
- Produces:
  - `int wmoToOwmId(int wmo)`: 0→800, 1→801, 2→802, 3→804, 45/48→741, 51→300, 53→301, 55→302, 56/57→511, 61→500, 63→501, 65→502, 66/67→511, 71→600, 73→601, 75→602, 77→600, 80→520, 81→521, 82→522, 85→620, 86→621, 95→211, 96/99→202, 97→212; anything else 800. Icons only need the family (`wxIconForId`), so exactness inside a family is not required.
  - `Tr wxOmDescTr(int owmId)`: translated condition text for an OpenWeather id (clear, partly cloudy, overcast, fog, drizzle, rain, freezing rain, showers, snow, thunderstorm, "--" fallback). Add the keys to `i18n.h` with EN and PT text.
  - `bool openMeteoParse(JsonDocument& doc, WeatherData& out, bool airOnly = false)`: fills `tempC, humidity, pressure, windMs, weatherId, iconCode` ("01d"/"01n" from `is_day`), `detailTempC/Humidity/WindMs/WeatherId/IconCode/Valid` columns 0 and 1 from `hourly` index 3 and 6, columns 2..3 from `daily` index 1..2 (the screen has 4 columns; `WX_DETAIL_COLS` 5 is not used for column 4) (max temp, `relative_humidity_2m_mean`, `wind_speed_10m_max`, daily `weather_code`, icon "01d"), `nightTempC[0..1]` from `temperature_2m_min`, and `fcWindMaxMs/fcWindGustMs` from hourly. `cityName` is left empty. With `airOnly` it only reads `current.pm2_5/pm10` from the air-quality response. Returns false and leaves `out` untouched on non-object, `error:true`, or missing `current.temperature_2m`.
  - `constexpr const char* OM_FORECAST_QUERY`: the exact query string below.
- The query: `current=temperature_2m,relative_humidity_2m,surface_pressure,wind_speed_10m,weather_code,is_day&hourly=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_gusts_10m&daily=weather_code,temperature_2m_max,temperature_2m_min,relative_humidity_2m_mean,wind_speed_10m_max&forecast_hours=7&forecast_days=3&timezone=auto&wind_speed_unit=ms`

- [ ] **Step 1: Write failing tests** in `test/test_wx_open_meteo/test_main.cpp`: `test_wmo_known_codes` (0→800, 3→804, 61→500, 95→211, 71→600), `test_wmo_unknown_is_clear` (100→800), `test_parse_sample` (embed a trimmed real response from the spec file; assert `tempC`, `humidity`, `pressure`, `weatherId`, `iconCode` "01d", `detailValid[0..3]` all true and `detailTempC[2]` equals the daily max index 1), `test_parse_night_icon` (is_day 0 gives "01n"), `test_parse_error_body_rejected` (`{"error":true,"reason":"x"}` returns false and `out` unchanged), `test_parse_short_hourly` (only 4 hourly entries: `detailValid[1]` false, `[0]` true), `test_parse_air_only` (`{"current":{"pm2_5":31.8,"pm10":32.0}}` sets pm25/pm10 and nothing else), `test_desc_fallback` (`wxOmDescTr(0)` is the generic key).
- [ ] **Step 2: Run** `pio test -e native -f test_wx_open_meteo`. Expected: FAIL (header missing).
- [ ] **Step 3: Implement** the three functions in `lib/core/wx_open_meteo.cpp` using ArduinoJson const variants like `weather_json.cpp`; add the `Tr` keys.
- [ ] **Step 4: Run** `pio test -e native`. Expected: all suites PASS.
- [ ] **Step 5: Commit** `feat: add Open-Meteo parser and WMO code mapping`.

---

### Task 2: Open-Meteo service and store slot

**Files:**
- Create: `src/svc/weather_om.h`, `src/svc/weather_om.cpp`
- Modify: `src/app/data_store.h` (add `Guarded<WeatherData> weatherOm;`), the place that calls `weatherTick` (grep it, likely `src/main.cpp` or the net task) to also call `weatherOmTick(now)`.

**Interfaces:**
- Consumes: `openMeteoParse`, `OM_FORECAST_QUERY` (Task 1); `serviceCoords`, `httpPollGate`, `httpGetJson`, `httpQuickRetryMs` as `src/svc/weather.cpp` uses them.
- Produces: `void weatherOmTick(uint32_t nowMs)`. It does nothing when WiFi is down, when the coordinates are unknown, or when the screens are disabled in `scr_en` (bits 10 and 11 both clear), so the API is not called for users who never enabled it. First fetch fires immediately once enabled. Fetches use a `JsonDocument` filter of the fields parsed. Sets `fetchedAt = now | 1`, `forecastFetchedAt = now | 1`, `valid = true`, `fetchFailed` and `forecastFetchFailed` like `weather.cpp`. The URL logs nothing sensitive (the coordinates are the only private part: never log the URL).

- [ ] **Step 1: Write the service**, mirroring `fetchCurrent` in `src/svc/weather.cpp` (static `Config`, static `WeatherData`, `reached()` helper). Air quality: every 6th successful cycle call `https://air-quality-api.open-meteo.com/v1/air-quality?latitude=%.4f&longitude=%.4f&current=pm2_5,pm10` and parse with `airOnly=true`; a failed air call does not fail the cycle.
- [ ] **Step 2: Build** `pio run -e cyd`. Expected: SUCCESS, record flash and RAM percentages.
- [ ] **Step 3: Commit** `feat: add Open-Meteo weather service`.

---

### Task 3: Screens, ids, menu and config

**Files:**
- Modify: `src/ui/screen.h` (`WeatherOm=10, ForecastOm=11`), `src/ui/screens/registry.cpp` (+ `SCR_CHECK`s), `src/ui/screens/weather_screen.{h,cpp}`, `src/ui/screens/forecast_screen.{h,cpp}`, `src/ui/screens/wx_common.{h,cpp}` (footer), `src/ui/screens/settings_screen.cpp`, `src/ui/ui_manager.cpp` if it lists ids, `lib/core/gesture.cpp` (`isCarouselScreen`: ids 1..12), `lib/core/gesture.h` (`scr::WeatherOm=10`, `scr::ForecastOm=11`), `lib/core/config_codec.cpp` (`kScrEnMask = 0x1FFE`), `lib/core/config.h` comment, `lib/core/i18n.h` (titles "Weather - Open-Meteo" / "Forecast - Open-Meteo" with PT)
- Test: `test/test_gesture`, `test/test_config_codec`

**Interfaces:**
- Consumes: `g_store.weatherOm`, `wxOmDescTr` (Tasks 1-2).
- Produces: `Screen* createWeatherScreen(bool openMeteo = false)` and `Screen* createForecastScreen(bool openMeteo = false)`. With `openMeteo` the class reports the new id, reads `g_store.weatherOm`, uses the `-Om` title key, never shows the "API key not set" message, shows `tr(wxOmDescTr(w.weatherId))` instead of `w.description`, and uses the footer `wxFooterOm(char*, size_t, const WeatherData&, bool forecast)`: "Open-Meteo.com  (N min)" or the no-data message. PM2.5/PM10 and the 4-column layout are unchanged.

- [ ] **Step 1: Write failing tests**: `test_gesture` (`isCarouselScreen(10)` and `(11)` true, `(13)` and `(14)` false; `nextScreen` includes 10 when enabled), `test_config_codec` (`scr_en` 0x3FFE clamps to 0x1FFE; default `scr_en` still 0x13FE; `screen_order` with 10 and 11 survives clamp, 13 is dropped; JSON import of old backups still works).
- [ ] **Step 2: Run** `pio test -e native -f test_gesture -f test_config_codec`. Expected: FAIL.
- [ ] **Step 3: Implement** the gesture and config changes, then the screen parameterisation and registry, settings arrays (`kScreenIds`, `kScreenNames`) and titles.
- [ ] **Step 4: Run** `pio test -e native` then `pio run -e cyd`. Expected: PASS and SUCCESS.
- [ ] **Step 5: Commit** `feat: add Weather and Forecast Open-Meteo screens`.

---

### Task 4: Web page, docs and tools

**Files:**
- Modify: `web/config.html` (`SCR` list gets `[10,"Weather - Open-Meteo"],[11,"Forecast - Open-Meteo"]` and the PT strings), `README.md` (feature list, "Weather data by Open-Meteo.com (CC-BY 4.0), air quality by CAMS", non-commercial and per-IP limit note), `docs/api.md` (`scr_en` range, `screen_order` values 0..12), `tools/screen_test.py` and `tools/check_text_fit.py` (new screens and their longest titles)

- [ ] **Step 1: Edit** the files above.
- [ ] **Step 2: Verify** `node --check` on the script extracted from `web/config.html` (the earlier GPS removal broke it once) and `python3 tools/check_text_fit.py`. Expected: no errors.
- [ ] **Step 3: Commit** `docs: document the Open-Meteo screens`.

---

### Task 5: Verification, upload, merge

**Files:** none.

- [ ] **Step 1: Gate** (Haiku): `pio test -e native` all PASS, `pio run -e cyd` SUCCESS, report flash and RAM percentages.
- [ ] **Step 2: Upload** (Haiku): `pio run -e cyd -t upload` with the board attached, then a 12 minute pyserial capture; expect two forecast fetches about 10 min apart after enabling the screens, no resets.
- [ ] **Step 3: Visual check by the user**: enable both screens in Settings, check the titles, footer and icons.
- [ ] **Step 4: Push** is outward-facing: ask the user first, then Haiku runs `git push origin main`.
