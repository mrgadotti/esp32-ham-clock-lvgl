# ESP32-HAM-CLOCK config/web findings

Source: `src/ESP32-HAM-CLOCK.cpp` (18615 lines, Arduino/PlatformIO, ESP32 4MB, WebServer + LittleFS, ArduinoJson 7). Line numbers below refer to it unless a file is named. `data/` files are served from LittleFS.

## 1. Preferences (NVS)

Single namespace **`dxcluster`** (read/write, `Preferences` begin at 16529 / 16754; also `saveBridgeBtPairingToNvs` 1986-1992 writes br_pair_* in the same namespace). All other `savePreferences()` callers (6978, 7566, 7672 ... 18xxx) just re-persist everything. `loadPreferences` is 16524-16747, `savePreferences` 16749-16860, called in setup at 18139. NVS key max is 15 chars.
Defaults (consts): 1223-1239, 1517-1522, 1538-1566, 1708-1722, 2383.

Legend: W = editable in web form (/api/config + /api/save). D = device-only (touch/TFT UI), not in the web form.

### WiFi (4 keys, W)
| key | type | default | normalization |
|---|---|---|---|
| wifi_ssid | String | "" | trim on save (17432+); save rejects if BOTH ssid empty (400 empty_ssid) |
| wifi_pass | String | "" | trim |
| wifi_ssid2 | String | "" | trim |
| wifi_pass2 | String | "" | trim |

### Station / locator / timezone (6 keys, W)
| key | type | default | notes |
|---|---|---|---|
| user_callsign | String | "" | trim; used for cluster login, APRS fallback callsign |
| user_locator | String | "" | trim; Maidenhead, >=4 chars used to derive lat/lon (updateUserLatLonFromLocator) |
| user_lat | Float | 0 | -90..90 validity check only on save |
| user_lon | Float | 0 | -180..180 |
| user_ll_ok | Bool | false | set true when lat AND lon both given and in range; if locator given and flag false, lat/lon derived from locator (17609-17612, 16716-16722). The web form always sends lat/lon (parseFloat||0), so 0,0 counts as "valid" -> locator never overrides. Surprise. |
| timezone | Float | 0.0 | clamp -12..14 (16572, 17160) |
| gps_auto_pos (NVS_KEY_USE_GPS_IF_ACTIVE) | Bool | false | "use GPS position if active" |

### DX cluster (9 keys, W)
| key | type | default | notes |
|---|---|---|---|
| cluster_host | String | dxspots.com | trim |
| cluster_port | Int | 7300 | web sends parseInt (NaN->not int-> reset to default 7300 server side) |
| cluster_noann | Bool | true | set/noann |
| cluster_nowwv | Bool | true | |
| cluster_nowcy | Bool | true | |
| cluster_usefl (NVS_KEY_CLUSTER_USEFILTERS) | Bool | true | JSON name `cluster_usefilters` |
| cluster_filters | String | "set/ft8" | newline-separated commands, trim only |

### POTA (4 keys; only pota_api_url is W)
| key | type | default | notes |
|---|---|---|---|
| pota_api_url | String | https://api.pota.app/v1/spots | W; empty -> default (17446) |
| pota_host | String | "" | accepted by /api/save and /api/config, but NOT in the web UI form (no input) |
| pota_port | Int | 7300 | accepted by API; web never sends it, so each web save resets it to 7300 (17613-ish: `else potaClusterPort = DEFAULT`) |
| pota_filter | String | "accept/spot comment POTA" | API only, not in UI |

### HamAlert (4 keys, W)
hama_host (def hamalert.org), hama_port (Int def 7300, 1..65535 else default), hama_user, hama_pass. If login OR password empty -> host cleared to "" and port to 0 = feature disabled (17449-17460). Web mirrors this in collectHamalertConfig (indexEN.html 2084).

### APRS-IS connection (7 keys, W)
| key | type | default | normalization |
|---|---|---|---|
| aprs_host | String | rotate.aprs2.net | trim |
| aprs_port | Int | 14580 | web: `parseInt || 14580`; server only checks is<int> (no range check) |
| aprs_callsign | String | "nocall" | trim; empty -> falls back to user_callsign (getAprsBaseCallsign 14783) |
| aprs_passcode | Int | 0 | web `parseInt||0` |
| aprs_ssid | Int | 0 | clamp 0..15 (normalizeAprsSsid 14767) |
| aprs_radius | Int | 10 | clamp 1..50 |
| (filter pos uses user_lat/lon, no separate keys) | | | |

### APRS TX beacon (4 keys, W)
aprs_beacon (Bool, default true), aprs_symbol (String 2 chars, default "/V"; sanitizeAprsSymbol 14747, invalid -> default), aprs_comment (String, sanitized printable ASCII, max 43 chars, 14796), aprs_int_min (Int, default 29, clamp 1..180, 15183).

### APRS alerts (8 keys, W)
| key | type | default | range |
|---|---|---|---|
| aprs_alert | String CSV | "" | sanitizeAprsAlertList: max 20 tokens (14839) |
| aprs_alert_en | Bool | true | |
| aprs_alert_near_en | Bool | true | |
| alrt_wx_en | Bool | false | APRS weather alerts |
| aprs_alrt_sec | Int | 300 | 0..86400 (min interval same station) |
| alrt_scr_s | Int | 60 | 1..60 (TFT alert time) |
| alrt_dst_km | Float | 1.0 | 0.1..500 |

### Weather alerts (3 keys, W)
wx_al_st (Bool false), wx_al_wi (Bool false), wx_w_thr (Float 12.0, clamp 1..60 m/s, 1609).

### LED alerts (3 keys, W)
led_al_en (Bool true), led_al_dur (Int 5000, clamp 100..60000), led_al_blk (Int 500, clamp 50..5000).

### QRZ / OpenWeather / SDR (4 keys, W)
qrz_user, qrz_pass (String ""), weather_key (String "" - OpenWeather API key), openwebrx_url (String, default `http://okno.ddns.net:8078`; UI label "KiwiSDR URL"). All trimmed except qrz_*. Also response-only `qrz_status` (read-only, not stored).

### UART2 / GPS / TNC / BT bridge (10 keys, W)
| key | type | default | normalization |
|---|---|---|---|
| uart2_mode | String | "gps" | codes gps/bt/off (uart2ModeFromCode ~2195); web derives it from connectedModule: gps->gps, aprs_tnc/aprs_tnc_kiss/esp32_bridge->bt, else off |
| uart2_cmod | String | "none" | sanitizeUart2ConnectedModule (1843): none / gps / aprs_tnc / aprs_tnc_kiss / esp32_bridge |
| uart2_baud | Int | 9600 | clamp 300..2000000 (2231) |
| uart2_dbits | Int | 8 | only 7 or 8 (7 else 8) |
| uart2_par | String | "none" | none/even/odd |
| uart2_stop | Int | 1 | 1 or 2 |
| br_pair_addr | String | "" | 12 hex chars (normalizeBridgeBtAddress); length 12 -> autoconnect at boot |
| br_pair_name | String | "" | max 40 chars. Both br_pair_* written by BT pairing flow, not by /api/save. |
Fixed pins (not configurable): RX IO27, TX IO22 (1699-1700), returned read-only in /api/config as uart2_rx_pin/uart2_tx_pin.

### TFT display / touch / theme / language / screens
| key | type | default | normalization | web? |
|---|---|---|---|---|
| tft_backlight | Int | 100 | clamp MIN_BACKLIGHT_PERCENT(10)..100 (UI slider says 0-100 but server min is 10) | W |
| tft_ndim | Int | 0 | 0..100 night dimming | W |
| tft_inv | Bool | per board define (671/673) | | W |
| tft_lang | UChar | 0 = PT_BR (TFT_LANG_PT_BR) | >EN -> PT_BR; JSON code "en"/"pt-br" | W |
| tft_style | UChar | dark | dark/light | W |
| tft_tbl_size | UChar | normal | normal/enlarged | W |
| menu_hue | Int | 20 | clamp 0..255 | W in API (menu_hue), no visible form input id | API only |
| tft_rot | UChar | 1 | 0..3 | W |
| touch_rot | UChar | 1 | 0..3 | W |
| touch_swap / touch_invx / touch_invy | Bool | false | derived from JSON `touch_swap_mode` (none, xy, x, y, both, rot90cw, rot90ccw, xy_both) | W (mode) |
| touch_xmin/xmax/ymin/ymax | Int | 200/3800/200/3800 | invalid range -> defaults (16623-16630) | D (on-device calibration; only reset via /api/reset_touch) |
| tft_autosw | Bool | false | | W |
| tft_sw_sec | Int | 30 | clamp 1..3600 (6691) | W |
| screen_order | Bytes (12 x ScreenType) | DEFAULT_SCREEN_ORDER (687) | ensureScreenOrderValid; unknown code -> default for slot; JSON array of codes: hamclock, dxcluster, aprsis, aprsradar, bandinfo, sunspots, weather, weatherforecast, lokalizacja, pota, hamalert, matrix, unlishunter(not in web dropdown), off | W (12 slots) |
| screen1_time | Int | UTC | UTC/LOCAL | D |
| s2_mode, s2_band, s6_sort, s7_mode, s7_band, s8_mode, s8_band | UChar/UInt | 0 / APRS_SORT_TIME | masks sanitized against allowed band bits | D (touch screen filters) |

### Counts (distinct NVS keys, total ~96)
WiFi 4, station 7, DX cluster 7, POTA 4, HamAlert 4, APRS-IS 6 (+aprs_beacon etc. below), APRS TX 4, APRS alerts 7, weather alerts 3, LED 3, QRZ/WX/SDR 4, UART/GPS/bridge 8, TFT display/touch/theme/order 23 (+ 11 device-only touch/screen-filter keys counted in that group's 23+). Web-editable subset is about 70 keys; device-only (touch cal 4 + s2/s6/s7/s8 7 + screen1_time 1) = 12.

## 2. Web server routes (setupWebServer 16877-17875, `WebServer(80)`, no auth, no CORS)

| Route | Method | Request | Response | Class |
|---|---|---|---|---|
| `/` and `/index.html` (16885-16894) | GET | | stream LittleFS /index.html (PT-BR) else getMainHTML() fallback | KEEP (new page replaces) |
| `/indexEN.html` (16897) | GET | | LittleFS indexEN.html, else index.html, else 404 | KEEP |
| `/config` (16951) | GET | | same as index.html | KEEP (alias) |
| `/manual-pt_br.txt`, `/manual.txt` | GET | | text/plain | KEEP optional |
| `/style.css`, `/style.base.css`, `/style.desktop.css`, `/style.mobile.css` | GET | | text/css | KEEP (reusable) |
| `/api/config` (17775) | GET | | flat JSON (4096 B doc) of all config + extras: see list below | KEEP |
| `/api/save` (17077) | POST application/json body ("plain") | flat JSON same keys as /api/config | `{"status":"ok","action":"restart"}` or 400 `{"status":"error","message":"json_parse"|"empty_ssid"}` | KEEP |
| `/api/reset_touch` (17639) | POST | none | `{"status":"ok"}` (no restart) | KEEP |
| `/api/reset_tft_rotation` (17754) | POST | none | `{"status":"ok","action":"restart"}`, restarts in 1.5 s | KEEP |
| `/api/uart_status` (17660) | GET | | `{uart2_mode, uart2_connected_module, gps_active, gps_fix, bt_active, aprs_uart_active}` | KEEP (status of config'd module; small) |
| `/api/bridge_bt_status` (17676) | GET | | `{enabled, scan_in_progress, list_collecting, list_expected, last_error, last_ack, disc_status, last_scan_ms, pair_pending, pair_result, pair_result_ms, paired_name, paired_addr, devices:[{name,addr,addr_raw}]}` | KEEP (pairing wizard) |
| `/api/bridge_bt_scan` (17706) | POST | none | 200 `{"status":"ok","action":"scan_started"}`; 400 `bridge_mode_required` | KEEP |
| `/api/bridge_bt_pair` (17717) | POST | `{name, address, password(<=16)}` | 200 `pair_started`; 400 `bridge_mode_required`/`missing_payload`/`json_parse`/`password_too_long`/BT error text | KEEP |
| `/api/spots` (16975) | GET | | `{spots:[{time,callsign,frequency,distance,country,band,mode,spotter,comment}]}` max 50 | DROP |
| `/api/aprs` (17000) | GET | | `{stations:[{time,callsign,symbol,lat,lon,comment,distance,hasLatLon,freq_mhz}]}` max 20 | DROP |
| `/api/pota` (17024) | GET | | `{spots:[...], count, total}` max 30 | DROP |
| `/api/hamalert` (17047) | GET | | same shape as pota | DROP |
| `/api/propagation` (17070-ish) | GET | | `{valid,sfi,kindex,aindex,muf,updated,lastError,hfBands:[{label,freq,day,night}]}` | DROP |

No `onNotFound` handler (unknown path -> default WebServer 404). No `/update`, no `/api/restart`, no `/api/reboot`, no `/api/wifi_scan`.

### /api/config JSON keys (17775-17870)
wifi_ssid, wifi_pass, wifi_ssid2, wifi_pass2, uart2_mode, uart2_baud, uart2_data_bits, uart2_parity, uart2_stop_bits, uart2_rx_pin, uart2_tx_pin (RO), cluster_host, cluster_port, pota_host, pota_port, pota_filter, pota_api_url, hamalert_host/port/login/password, user_callsign, user_locator, timezone, user_lat, user_lon, user_lat_lon_valid (RO on save: ignored), use_gps_if_active, uart2_connected_module, bridge_paired_name, bridge_paired_addr (RO), qrz_user, qrz_pass, weather_key, openwebrx_url, tft_backlight, tft_night_dimming, tft_invert, tft_rotation, tft_lang ("en"/"pt-br"), tft_style ("dark"/"light"), table_size ("normal"/"enlarged"), tft_auto_switch, tft_switch_time_sec, menu_hue, qrz_status (RO), cluster_noann/nowwv/nowcy/usefilters/filters, aprs_host/port/callsign/passcode/ssid/radius/beacon/symbol/comment, aprs_alert, aprs_alert_enabled, aprs_alert_nearby_enabled, aprs_alert_wx_enabled, weather_alert_storm_enabled, weather_alert_wind_enabled, weather_alert_wind_threshold_ms, aprs_alert_min_sec, aprs_alert_screen_sec, aprs_alert_distance_km, enable_led_alert, led_alert_duration_ms, led_alert_blink_ms, aprs_interval_min, touch_swap_mode, touch_rotation, screen_order[12].

### /api/save behaviour (17077-17636)
- Body read via `arg("plain")`; `StaticJsonDocument<8192>`; parse error -> 400 json_parse.
- **Field-presence semantic is inconsistent**: most keys guarded with `is<T>()` (absent -> unchanged), BUT these are applied unconditionally with `as<String>()` so a partial POST wipes them to "": wifi_ssid, wifi_pass, cluster_host, user_callsign, user_locator. Also cluster_port/pota_port default if absent/non-int. `hamalert_*` are cleared if login/password empty. The new page MUST send the full payload (or the firmware should be hardened).
- Type checks: bools need real JSON bool, ints real int (floats like `5.0` rejected for aprs_port, aprs_passcode, hamalert_port, cluster_port, backlight etc.). Numeric fields with a `containsKey` guard accept int/long/float/string (uart2_baud, data/stop bits, aprs_alert_*, led_*, aprs_interval_min, weather wind threshold, tft_switch_time_sec). user_lat/lon require float/double (JSON integer 0 would be ignored: `is<float>` on an int in ArduinoJson 7 is true for ints? ArduinoJson `is<float>()` returns true for ints too, so OK).
- Normalization: see tables above (clamps, trims, sanitizers). SSID is the only hard validation: both SSIDs empty -> 400 empty_ssid (note: state already mutated in RAM before the check, but NVS not written).
- No validation of host format, port range for cluster/aprs, locator syntax, callsign syntax, URL scheme.
- On success: `savePreferences()`, `setBacklightPercent`, `applyTftInversion`, responds `{"status":"ok","action":"restart"}`, then `requestRestart(1500)` -> device always reboots 1.5 s after any save (17630-17636). The loop executes restart via restartRequested (2415-2417).
- `screen_order`: array of strings (codes) or ints; missing tail filled with defaults.
- /api/reset_touch resets touch cal + rotation, saves, no restart. /api/reset_tft_rotation sets rot 1, restarts.

## 3. Web UI (data/index.html PT-BR 148836 B / 2693 lines; data/indexEN.html 145287 B / 2683 lines)

- Approach: single-page, vanilla JS (no framework, no build, no npm), inline `<script>` blocks (indexEN 1071-2670; index.html has two script blocks 972-1873 and 1972-2680), `fetch()` + JSON, `showPage(id)` tab switching, CSS from three LittleFS files (style.base.css 7.7 KB, desktop 9.3 KB @min-width 901px, mobile 12.6 KB @max-width 900px). Dark theme. PT and EN are separate near-identical copies (same element ids; ids diff is empty). Language switch is just a link to `/index.html` vs `/indexEN.html`.
- External calls from browser: api.github.com releases/latest (version check, 1127), qrz.com and aprs.fi links, `/manual.txt` fetch. Coffee link buycoffee.to.
- Polling (all DROP): loadSpots 2s, loadAPRS 2s, loadPropagation 60s, loadPOTA 60s, loadHamalert 60s, loadUart2Status 2s (KEEP-able), loadBridgeBtStatus 3s (only when bridge selected).
- Settings backup: export/import of current form to TXT as JSON envelope `{format:"ESP32-HAM-CLOCK-config", version:1, exportedAt, config:{...collectConfigPayload}}` (client-side only, includes all passwords in clear). Good feature to KEEP.

### Tabs
| Nav button / page id | Content | Class |
|---|---|---|
| DXSPOT `dxspot-page` | spot table, band/mode/continent filters (cont_af..cont_oc checkboxes are spot FILTERS, not prefs) | DROP |
| POTA `pota-page` | POTA spots table + filters | DROP |
| HAMALERT `hamalert-page` | HamAlert spots | DROP |
| APRS `aprs-page` | APRS stations table | DROP |
| Propagation `propagation-page` | SFI/K/A/MUF + HF band info | DROP |
| ALERT `alert-page` | APRS ALERT + LED + weather alert form (aprs_alert*, led_alert*, weather_alert*) | KEEP (config) |
| TFT Screen `tft-page` | backlight, night dim, language, style, table size, screen order slots 1-12, auto-switch, TFT/touch rotation, invert, touch swap mode, reset buttons | KEEP (config; "TFT mirror" here is not a mirror, just settings) |
| Settings `config-page` | form `#config-form`: WiFi, UART, DX cluster+filters, POTA API URL, HamAlert, Station, QRZ, OpenWeather, KiwiSDR, APRS-IS, Settings Backup | KEEP |
| Modules (`modules-page`, injected by JS `ensureModulesTabEN` ~2570) | connected module select + UART params (moves `uart-settings-section` in) + BT SPP scan/pair list | KEEP |
| Manual `manual-page` | renders manual.txt | KEEP optional |
| Links `links-page` | static links + firmware version | DROP/optional |

### Form field ids (id == name == JSON key == pref key unless noted)  - 86 input/select/textarea elements total in indexEN.html, 7 of which are the cont_* spot filters, 1 file input, 12 screen_slot selects.
Config fields (about 66 unique, excluding qrz_status RO):
- WiFi: wifi_ssid, wifi_pass, wifi_ssid2, wifi_pass2
- UART/module: connectedModule (select; maps to uart2_connected_module + derives uart2_mode), uart2_baud, uart2_data_bits, uart2_parity, uart2_stop_bits
- DX cluster: cluster_host, cluster_port, cluster_noann, cluster_nowwv, cluster_nowcy, cluster_usefilters, cluster_filters (textarea)
- POTA: pota_api_url
- HamAlert: hamalert_host, hamalert_port, hamalert_login, hamalert_password
- Station: user_callsign, user_locator, timezone (step 0.5, -12..14), user_lat, user_lon, use_gps_if_active
- QRZ/WX/SDR: qrz_user, qrz_pass, qrz_status (RO), weather_key, openwebrx_url
- APRS-IS: aprs_host, aprs_port, aprs_callsign, aprs_ssid (select 0-15), aprs_passcode, aprs_radius (UI min 0 max 50; server clamps 1..50), aprs_beacon, aprs_symbol (maxlength 2), aprs_comment (maxlength 43), aprs_interval_min
- Alerts: aprs_alert, aprs_alert_enabled, aprs_alert_nearby_enabled, aprs_alert_wx_enabled, aprs_alert_min_sec, aprs_alert_screen_sec, aprs_alert_distance_km, enable_led_alert, led_alert_duration_ms, led_alert_blink_ms, weather_alert_storm_enabled, weather_alert_wind_enabled, weather_alert_wind_threshold_ms
- TFT: tft_backlight (range), tft_night_dimming (range), tft_lang, tft_style, table_size, tft_rotation, tft_invert (select "0"/"1" sent as bool), tft_auto_switch, tft_switch_time_sec, touch_rotation, touch_swap_mode, screen_slot_1..12 (-> screen_order[])
- Not in the UI but in the API/NVS: pota_host, pota_port, pota_filter, menu_hue (sent by neither form; /api/save leaves menu_hue alone but resets pota_port).
Client-side clamps mirror server ones (collectConfigPayload 2099-2237).

### getMainHTML fallback
`getMainHTML()` (17879-17882) is a 2-line stub `"index.html missing in LittleFS"` (~110 bytes of flash). `getConfigHTML()` calls it. There is **no embedded config page** in firmware; without LittleFS data upload the UI shows only that message (and `/api/*` still work). `LittleFS.begin(true)` at 18108 (formats on failure). Partition: spiffs 0x130000 (1.2 MB) (partitions_4mb_ota_1600k.csv). platformio.ini extra script `erase_fs.py`.
data/ total size: ~148 KB + 145 KB HTML + 30 KB CSS + 36 KB manuals + logo.bmp 307 KB + fonts/ icon50/ dirs (not measured).

## 4. AP fallback, OTA, security

### AP fallback / captive portal
- `startAPMode()` (15930-15947): `WiFi.mode(WIFI_AP)`; `softAP("ESP32-HAM-CLOCK", "1234567890")` (hardcoded, `#define AP_SSID/AP_PASSWORD` at 1223-1224, WPA2 fixed password, same on every device). IP is default 192.168.4.1.
- Entered when: no wifi_ssid saved at boot (18176-18180), or connect fails after attempts (18176), or later via reconnect logic (16099, 16124). AP-only (no STA) so the portal is stable. While in AP mode a STA retry is attempted every WIFI_AP_RETRY_INTERVAL_MS = 5 min (2396, 16086). On STA success the AP is shut down (18283-18285).
- **No captive portal**: no DNSServer, no redirect/onNotFound; user must browse to 192.168.4.1 manually. No mDNS either (grep for MDNS found nothing). IP is shown on TFT.
- After a save the device reboots (always).

### OTA
- **No OTA code**: no ArduinoOTA, `Update.h`, HTTPUpdate or `/update` route. The partition table `partitions_4mb_ota_1600k.csv` has two 1.5 MB OTA slots (app0/app1 + otadata) but is unused. Firmware is flashed via USB (`flash/` folder in repo). The web UI only checks GitHub for latest release version string (display).

### Security
1. No authentication at all on any route (HTTP, port 80, plain text). Anyone on the LAN or on the AP can read and overwrite config, trigger restart, start BT scan/pair.
2. `/api/config` returns secrets in clear: wifi_pass, wifi_pass2, hamalert_password, qrz_pass, weather_key, aprs_passcode. Password inputs in the UI are populated from it. Exports to TXT contain them in clear.
3. Default AP uses a published fixed WPA2 password "1234567890" and fixed SSID (source is public).
4. No CSRF/origin check; POST JSON with `fetch` from any web page on the LAN can be sent with content-type text/plain (server reads `plain` arg regardless) -> CSRF possible.
5. No rate limit; no input length limits on string fields except UI maxlength (APRS comment sanitised server-side). StaticJsonDocument<8192> bounds payload size (oversized -> json_parse 400).
6. `/api/save` mutates RAM state before validating SSID (400 on empty SSID leaves RAM changed until reboot).
7. Partial-payload wipe risk (callsign, locator, ssid, pass, cluster_host) described above.
8. Fixed DEFAULT_OPENWEBRX_URL points to a personal DDNS host (okno.ddns.net:8078), ships as default.
9. Flash: a trailing "restart" always follows save, so any client can DoS by POST of an empty-SSID-less payload repeatedly? (POST of `{}` returns 400 empty_ssid since both SSIDs would be wiped; payload must contain SSID).

### Surprises / notes for migration
- Single NVS namespace "dxcluster" (kept as-is to preserve user settings across the firmware swap).
- JSON field names != NVS key names for several (e.g. `aprs_alert_enabled`->aprs_alert_en, `hamalert_host`->hama_host, `tft_invert`->tft_inv, `tft_rotation`->tft_rot, `table_size`->tft_tbl_size, `tft_night_dimming`->tft_ndim, `tft_switch_time_sec`->tft_sw_sec, `enable_led_alert`->led_al_en, `use_gps_if_active`->gps_auto_pos, `uart2_data_bits`->uart2_dbits, `aprs_interval_min`->aprs_int_min, `weather_alert_*`->wx_al_*). `uart2_connected_module` also depends on `uart2_mode` derivation in the client.
- Default tft_lang is PT-BR in firmware and PT-BR index.html is `/`; EN is a separate file.
- Two screen-order concepts: SCREEN_PAGE_COUNT=12 slots but 13 screen types (unlishunter exists in firmware but is not offered in the web dropdown).
- Screen filters (s2/s7/s8 masks, s6 sort, screen1_time) and touch calibration min/max are device-only; a config-only web page may leave them out.
- `aprs_radius` UI allows 0, server clamps to 1.
- Tabs cut for the new page: DXSPOT, POTA, HAMALERT, APRS, Propagation, Links (and endpoints /api/spots, /api/aprs, /api/pota, /api/hamalert, /api/propagation). The cont_* checkboxes belong to the DX spot filter and are not preferences.
- The `/api/uart_status` poll (2 s) is a small status; keep if the module status line is wanted.
