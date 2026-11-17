# Screen test report

Board: ESP32 CYD 320x240, firmware = main at `b4f3d24` (settings reset fix) + `SCREEN_TEST` harness (env `cyd_test`).
Config under test: callsign PP5MGT, locator GG53ie, OpenWeather key; QRZ/HamAlert logins empty.
Method: `tools/screen_test.py` over `/dev/ttyUSB0` @ 921600 (full frame in ~2.5 s), PNGs reassembled from the
display flush stream and inspected by eye; touch is injected through the indev read callback.
Screenshots: `docs/review/screens/`, contact sheet `docs/review/screens/contact_sheet.png`
(top 3 rows EN, bottom 3 rows PT). Serial logs: `docs/review/serial.log`, `docs/review/soak.log`.

Result: **7 screens PASS, 5 FAIL** (12 screens). Soak: no panic / reboot / lvfree < 8 KB, but boot-time SSL allocation errors recur.

## Per-screen table

| Screen (id) | EN | PT | Result | Defects |
|---|---|---|---|---|
| HamClock (1) | `hamclock_en.png` | `hamclock_pt.png` | PASS | D9 (cosmetic) |
| DxCluster (2) | `dx_en.png` | `dx_pt.png` | FAIL | D1 (high), D4 (low) |
| SunSpots (3) | `sunspots_en.png` | `sunspots_pt.png` | PASS | none |
| BandInfo (4) | `bands_en.png` | `bands_pt.png` | PASS | none |
| Weather (5) | `weather_en.png` | `weather_pt.png` | PASS | D8 (low) |
| AprsList (6) | `aprslist_en.png` | `aprslist_pt.png` | FAIL | D1 (high) |
| Pota (7) | `pota_en.png` | `pota_pt.png` | FAIL | D1 (high), D4 (medium) |
| HamAlert (8) | `hamalert_en.png` | `hamalert_pt.png` | FAIL | D1 (high), D3 (medium) |
| AprsRadar (9) | `aprsradar_en.png` | `aprsradar_pt.png` | FAIL | D5 (medium), D7 (low, unconfirmed), D10 (unconfirmed) |
| Forecast (12) | `forecast_en.png` | `forecast_pt.png` | PASS | D8 (low) |
| Location (13) | `location_en.png` | `location_pt.png` | PASS | none ("GPS: INACTIVE / INATIVO" shown) |
| Settings (14) | `settings_en.png` | `settings_pt.png` | PASS | D9 (cosmetic) |

## Checklist results

| Check | Result |
|---|---|
| Headers CALL/MOD/MHz/UTC, MHz with 3 decimals (7.048, 14.320, 28.075, 21.074) | PASS |
| Zebra rows (gray / none) | PASS on POTA (`pota_en.png`); DX had only 1-2 spots, not verifiable |
| Light-gray 1 px separator under column headers | PASS: single row y=43, RGB(214,211,214), on DX/POTA/HamAlert/APRS |
| Tabular digits: clock `HH:MM:SS` glyph columns over 5 shots 1 s apart | PASS: colon and digit cells at identical x (56-81, 96-106, 120-124, ... 239-263); only the glyph inside a cell changes |
| Slider knobs fully visible at both ends (`s_bright_min/max`, `s_ndim_min/max`) | PASS |
| Header title vs clock text | PASS (no overlap on any screen) |
| PT-BR accents (ç, ã, õ, é, à) | PASS: CONFIGURAÇÕES, PROPAGAÇÃO, Terça, ESTAÇÃO, NÃO, localização |
| Weather icon + footer age | PASS: icon, "Weather for location: Timbó (0 min)" |
| Radar rings labelled range and range/2 km, "1×" button, zoom indicator | PASS (20.0km / 10.0km, "1×", header shows "×2/×4/×8"); label overlap D5 |
| Location shows GPS inactive without module | PASS |
| Settings has no reset (scrolled to bottom: SSID/IP/RSSI/heap/firmware only, `s_bottom.png`) | PASS |
| Swipe left/right changes screens (HamClock -> DX -> HamClock -> Settings wrap) | PASS (`i_swipe_*`) |
| Vertical drag scrolls tables (POTA) and Settings list | PASS (`i_pota_scrolled.png`, `s_scroll*.png`) |
| DX filter menu opens/closes, mode tile toggles, CANCEL and OK close | PASS, but only when tapping outside the hamburger bars (D1) |
| APRS list menu opens, CANCEL closes (OK/checkboxes not touched) | PASS (same D1 caveat) |
| Radar tap zoom x2/x4/x8 and "1×" reset | PASS (`i_radar_*`; the shot names lag by one tap, see D7) |
| Settings dropdown opens (`s_dd_open.png`); language switch rebuilds screen in PT (`s_lang_pt.png`) | PASS |

## Defects

| # | Sev | Screen(s) | Description | Suspects |
|---|---|---|---|---|
| D1 | high | DX, POTA, HamAlert, APRS list | The hamburger button does not open the menu when the tap lands on the icon. The three 20x3 bars are clickable children of the 40 px button without `EVENT_BUBBLE`, so a tap at (20,13) (middle bar) is swallowed; only taps in the 3 px gaps or x >= 31 work (tap at (35,13) opens it). About 60 % of icon taps hit a bar. Reproduced with tap (20,13) 3 times. | `src/ui/screens/dx_screen.cpp:49-56`, `src/ui/screens/spot_list_screen.cpp:49-56`, `src/ui/screens/aprs_list_screen.cpp:85-92`; fix: `lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE)` or `lv_obj_add_flag(bar, LV_OBJ_FLAG_EVENT_BUBBLE)` |
| D2 | high | all (network) | `SSL - Memory allocation failed (-32512)` on 6 of 19 logged boots, 10-11 s after boot: the OpenWeather and hamqsl fetches fail with `-1` (`largest=32756->32756`) right after POTA's TLS session and the first screen build (largest block drops 49140 -> 32756). Data arrives on the next retry, but this is the "SSL allocation error" class the soak must flag. | `src/net/http_fetch.cpp` (no largest-block guard / backoff before connect), `src/ui/ui_manager.cpp:49-60` (first screen allocated at the same moment), `src/svc/weather.cpp`, `src/svc/propagation.cpp` |
| D3 | medium | HamAlert (PT) | "LOGIN HAMALERT NÃO DEFINIDO" in the 18 px font is wider than 320 px and is clipped on both sides (`hamalert_pt.png`). EN is fine. | `lib/core/i18n.h:51` (`HamAlertNoLogin`), label at `src/ui/screens/spot_list_screen.cpp:72-85` (no width / wrap / long mode) |
| D4 | medium | POTA, DX | UTC column too narrow: "0146" touches the callsign ("0146KD2ZZA", no gap; DX shows 1-2 px). | column widths `{38, 76, ...}` at `src/ui/screens/spot_list_screen.cpp:38`, `src/ui/screens/dx_screen.cpp:38` |
| D5 | medium | APRS radar | Compass label "N" overprints the outer ring label "20.0km" at zoom 1x (`aprsradar_en.png`). The "N" is drawn unconditionally and is not checked against used boxes; the range label is placed at qx+4 just right of the same spot. | `src/ui/screens/aprs_radar_screen.cpp:168,172` |
| D6 | low | APRS radar | At x8 the QTH and both rings are off screen and nothing is drawn but "N" and the "1×" button; no hint where the station went (zoom is centred on the tap point). | `src/ui/screens/aprs_radar_screen.cpp:107`, `lib/core/radar_proj.cpp:15` |
| D7 | low, unconfirmed | APRS radar | In one run the first tap after `goto 9` did not zoom (3 taps gave x4, not x8); in another run 3 taps gave x8. Could be an injected-touch artifact; not root-caused. | `src/hal/touch.cpp` readCb, `aprs_radar_screen.cpp:43` (SHORT_CLICKED) |
| D8 | low | Weather, Forecast | Weather: PM2.5/PM10 row sits 1 px under the WIND row (visually crowded, uneven spacing). Forecast: "NIGHT 22°" labels in the Tue/Wed columns reach the column separators. No overlap or clipping. | `src/ui/screens/weather_screen.cpp`, `src/ui/screens/forecast_screen.cpp` |
| D9 | cosmetic | HamClock, headers, Settings | Tabular digit cell leaves a visible hole around "1" ("0 1:47 Z", "1 0 1 2 hPa", "192.168.0.1 3 1"). Settings label casing is inconsistent: BRIGHTNESS and LANGUAGE are upper case, the other rows title case. | `src/ui/fonts/` tabular digits; `src/ui/screens/settings_screen.cpp` / `lib/core/i18n.h` |
| D10 | high, unconfirmed | APRS radar | One run: after the third zoom tap (x8) the UI task stopped (no further `shot`/`mem` reply, no panic/Guru, net task kept logging, `uistk` ~2.7 KB). Re-running the same sequence twice did not reproduce it. Possibly an LVGL assert loop (arc radius 800 px at x8) or a harness effect. | `src/ui/screens/aprs_radar_screen.cpp:100-106,164-166` (arcs with r = 400/800), LVGL `LV_ASSERT` handler (`include/lv_conf.h`) |

Notes:
* The PT weather description stays English ("overcast clouds") only because the harness switches language at runtime and the data was fetched with `lang=en`; not a firmware defect (`src/svc/weather.cpp:35` sends `pt_br` for PT).
* The beacon was already enabled in the user's config (`[APRS] beacon sent` at ~68 s after every boot). Nothing in the test touched it (APRS menu was opened and cancelled; OK/checkboxes never tapped).
* DX/APRS/HamAlert have little or no data in the first minutes: "WAITING FOR SPOTS..." / "WAITING FOR APRS..." / "HAMALERT LOGIN NOT SET" are expected.
* Opening `/dev/ttyUSB0` resets the CYD (DTR/RTS); the host tool waits 16 s for the boot screen. SSL errors in D2 occur in that boot window.
* Persistent settings changed by the interaction tests (language dropdown, brightness and night-dimming sliders) were restored through `/api/save` (`tft_lang=en`, `tft_backlight=39`, `tft_night_dimming=0`) and read back; invert, rotation, auto-switch and screen mask were never changed. Harness `lang`/`autosw` commands only change the RAM snapshot.

## Interaction shot names (in `docs/review/screens/`)

`i_dx_menu*`, `i_dx_after_cancel/ok`: DX filter menu. `i_pota_*`: scroll. `i_aprs_*`: APRS menu. `i_swipe_*`: carousel
(`i_swipe_left` = DX, `i_swipe_right` = HamClock, `i_swipe_right2` = Settings). `i_radar_x2/x4/x8/reset` were taken after
taps 1-4; because of D7 their zoom is one step behind (x2 = 1x, x4 = x2, x8 = x4, reset = 1x). `s_*`: Settings (sliders, dropdown, language, scroll).

## Soak (10 minutes, `goto` every 10 s over all 12 screens, `mem` per screen)

60 screen visits (5 per screen), no panic, abort, Guru, `rst:` or reboot (one `[ST] ready` only), no task watchdog.
Only SSL errors: the two boot-time ones (D2, t = 11 s, before the first `goto`). Smallest lvfree seen 18740 B (Settings),
all other screens >= 21.6 KB (threshold 8 KB), no downward trend (per-screen min/max within 0.3 KB); minfree heap 51048 B; largest block min 18420 B (during a TLS fetch).

| Screen | lvfree min/max (B) | largest min (B) |
|---|---|---|
| 1 HamClock | 29616 / 29824 | 32756 |
| 2 DX | 28008 / 28620 | 32756 |
| 3 SunSpots | 28000 / 28084 | 32756 |
| 4 BandInfo | 25884 / 26032 | 32756 |
| 5 Weather | 27080 / 27164 | 32756 |
| 6 AprsList | 28016 / 28508 | 18420 |
| 7 Pota | 25756 / 26104 | 32756 |
| 8 HamAlert | 28428 / 28512 | 32756 |
| 9 AprsRadar | 30340 / 30376 | 32756 |
| 12 Forecast | 21660 / 21700 | 32756 |
| 13 Location | 29976 / 30040 | 32756 |
| 14 Settings | 18740 / 18856 | 32756 |

Soak verdict: PASS for the lvfree / panic / reboot criteria, FAIL for "SSL allocation error" (D2 appears at boot, not during the cycling).

## How to rerun

```
pio run -e cyd_test -t upload --upload-port /dev/ttyUSB0
python3 tools/screen_test.py --all-screens            # EN + PT shots of every screen
python3 tools/screen_test.py tools/screen_test_interact.txt
python3 tools/screen_test.py -c "soak 600 10"
```
Serial commands: `goto <id>`, `shot`, `tap x y`, `swipe x1 y1 x2 y2 [ms]`, `lang en|pt`, `autosw 0|1`, `mem`, `ping`.
