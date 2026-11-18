# HTTP API reference

The board runs a small HTTP server on port 80 (`src/net/web_config.cpp`). It
is reachable on the LAN at the board's station IP, and in access point mode at
`http://192.168.4.1`. The server is started once WiFi (STA or AP) is up.

The API has **no authentication**. Anyone who can reach the board can read the
configuration (including the backup export, which contains every secret in
plain text) and change settings. Keep the board on a trusted LAN, or only use
the AP while setting it up.

Only the routes listed here exist. Any other path returns the WebServer default
`404 Not found: <path>` (`text/html`). A known path with the wrong method also
returns 404.

## Conventions

- All bodies are JSON. Responses use `Content-Type: application/json` unless
  noted.
- **POST requests** (`/api/save`, `/api/reset_touch`, `/api/bridge_bt_scan`,
  `/api/bridge_bt_pair`) must send `Content-Type: application/json`, or the
  board answers `415 {"error":"content_type_must_be_json"}`. This blocks
  cross-site form posts (a browser cannot send a JSON content type without a
  CORS preflight, which the server never answers).
- If an `Origin` header is sent it must equal `http://<Host>`, and the `Host`
  must be an IP literal (starting with a digit). Otherwise the board answers
  `403 {"error":"bad_origin"}`. curl and other non-browser clients normally
  send no `Origin` and pass.
- Bridge POST routes report errors as `{"status":"error","message":"..."}`
  instead of `{"error":...}`. Save errors use `{"error":"..."}`.

### Routes

| Method | Path | Purpose | Auth |
|---|---|---|---|
| GET | `/` | Configuration web page (gzip) | none |
| GET | `/api/config` | Current settings, secrets masked | none |
| POST | `/api/save` | Partial update of settings | none |
| GET | `/api/backup` | Full settings export, **secrets in plain text** | none |
| POST | `/api/reset_touch` | Clear touch calibration, start it on the screen | none |
| GET | `/api/status` | IP, RSSI, uptime, heap, version, QRZ status | none |
| GET | `/api/uart_status` | UART2 module and BT state | none |
| GET | `/api/bridge_bt_status` | Bluetooth bridge state and device list | none |
| POST | `/api/bridge_bt_scan` | Start a Bluetooth scan (bridge mode only) | none |
| POST | `/api/bridge_bt_pair` | Pair with a device (bridge mode only) | none |

## GET /

Returns the configuration page (`web/config.html`, gzipped at build time by
`tools/gen_web.py`).

- Status `200`, `Content-Type: text/html`, `Content-Encoding: gzip`.
- The page is a single file with inline CSS and JavaScript. It calls the API
  routes below.

```sh
curl -s --compressed http://BOARD_IP/ -o config.html
```

## GET /api/config

Returns every JSON-exposed setting (see the field reference below). Secret
strings are **not** returned. For each secret field `<name>` the response has a
boolean `<name>_set` instead.

Response `200`:

```json
{
  "wifi_ssid": "MyHomeWiFi",
  "wifi_pass_set": true,
  "wifi_ssid2": "",
  "wifi_pass2_set": false,
  "ap_pass_set": true,
  "user_callsign": "N0CALL",
  "user_locator": "AA00aa",
  "user_lat": 0.0,
  "user_lon": 0.0,
  "user_lat_lon_valid": true,
  "timezone": -3.0,
  "tft_lang": "en",
  "tft_rotation": 1,
  "screen_order": [1, 2, 6, 9, 4, 3, 5, 12, 7, 8, 0, 0]
}
```

(Abridged. The real response contains all exposed fields.)

Errors: `500 {"error":"config_too_large"}` (should not happen with the current
field set).

```sh
curl -s http://BOARD_IP/api/config
```

## POST /api/save

Applies a **partial update**. Only the keys present in the body are changed.
Keys that are absent are left as they are, and unknown keys are ignored.

Request: `Content-Type: application/json`, body up to 6144 bytes.

```json
{"user_callsign": "N0CALL", "cluster_port": 7300, "wifi_pass": "example"}
```

Rules:

- **Secret fields** (`wifi_pass`, `wifi_pass2`, `ap_pass`,
  `hamalert_password`, `aprs_passcode`, `qrz_pass`, `weather_key`): an empty
  string keeps the stored value. To change a secret, send the new value.
- **Read-only**: `user_lat_lon_valid` is ignored on save.
- **Clamping**: numbers outside the field's range are clamped to the range, not
  rejected (see the field reference). `tft_rotation` other than 1 or 3 becomes 1.
- **Strings**: trimmed of surrounding whitespace, truncated to the field size,
  and non-printable ASCII is replaced by `?` (the WiFi SSIDs keep UTF-8 and drop
  control bytes; `cluster_filters` keeps newlines).
- **Type errors** are rejected (see below). The whole request is all-or-nothing:
  if any field fails validation, nothing is changed.
- **Restart**: if a changed field is marked *restart* in the field reference
  (WiFi, AP password, UART2 settings), the response has `"restart": true` and
  the board reboots about 1.5 s after the response. The web page then reloads
  after 8 s.

Responses:

| Status | Body | Meaning |
|---|---|---|
| 200 | `{"status":"ok","restart":false}` | Saved and applied live |
| 200 | `{"status":"ok","restart":true}` | Saved; the board restarts in about 1.5 s |
| 400 | `{"error":"json_parse:"}` | Body is not a JSON object (empty or invalid JSON) |
| 400 | `{"error":"bad_type:<key>"}` | Wrong JSON type for `<key>` (for example a string for a number) |
| 400 | `{"error":"bad_value:<key>"}` | Invalid enum value; only `tft_lang` (`en` or `pt-br`, case-insensitive) |
| 400 | `{"error":"too_big:<key>"}` | Internal field size limit (should not happen) |
| 400 | `{"error":"nvs_save"}` | Value applied in RAM but could not be written to flash |
| 413 | `{"error":"body_too_large"}` | Body over 6144 bytes |
| 415 | `{"error":"content_type_must_be_json"}` | Missing or non-JSON `Content-Type` |
| 403 | `{"error":"bad_origin"}` | `Origin` or `Host` check failed |

```sh
curl -s -X POST http://BOARD_IP/api/save \
  -H 'Content-Type: application/json' \
  -d '{"user_callsign":"N0CALL","tft_backlight":80}'
# {"status":"ok","restart":false}

curl -s -X POST http://BOARD_IP/api/save \
  -H 'Content-Type: application/json' \
  -d '{"wifi_ssid":"ExampleNet","wifi_pass":""}'
# {"status":"ok","restart":true}   (empty wifi_pass keeps the stored password)
```

## GET /api/backup

Full export of every setting, **including all secrets in plain text**, for
restoring on another board.

- Status `200`, `Content-Type: application/json`,
  `Content-Disposition: attachment; filename=esp32-ham-clock-backup.json`,
  `Cache-Control: no-store`.
- Keys are the same as `/api/config`, but the secret fields are real values
  instead of `*_set` flags, plus `last_scr` (see below).
- The response is built in a shared buffer that is wiped after sending. It is
  never logged.
- Error: `500 {"error":"config_too_large"}`.

### What the backup contains

Every persisted value (NVS namespace `hamclk`), nothing else is stored:

- All 74 table fields (including `tcal` below): WiFi and AP (SSIDs, passwords), station (callsign,
  locator, lat/lon, timezone), DX cluster, HamAlert, APRS-IS
  and beacon, alert and LED settings, QRZ and OpenWeather credentials, UART2 and
  bridge settings, Bluetooth pairing, display (backlight, night dimming, invert,
  language, rotation, auto-switch), `screen_order`, `scr_en`, the per-screen
  filters (`s2_*`, `s7_*`, `s8_*`, `screen1_time`) and `aprs_sort`.
- `tcal`, the touch calibration of the panel (per device, see below).
- `last_scr`, the last manually selected screen (NVS key `last_scr`, not part of
  the table). Always a valid screen id (never 0 or Settings); on
  import anything else is rejected with `400 bad_value:last_scr`.

Before this, the backup lacked `tcal`, `last_scr`, `screen1_time`, `s2_*`,
`s7_*`, `s8_*`, `aprs_sort` and `br_pair_*`. Old backups without them still
import, and keys the firmware does not know are ignored. `/api/config` also
returns the table keys above (including `tcal`) but never `last_scr` or secrets.

The touch calibration belongs to THIS panel. If touch is off on the new board,
recalibrate with a 15 s hold on the clock screen, or from Settings.

> **Security warning.** The backup file contains WiFi passwords, the AP
> password, the HamAlert password, the APRS-IS passcode, the QRZ password and the
> OpenWeather API key in plain text. Anyone who gets the file, or can reach the
> board on the LAN, can read them. Store the file like a password file, and do
> not paste it into chats, issues or screenshots.

```sh
curl -s -o esp32-ham-clock-backup.json http://BOARD_IP/api/backup
```

## POST /api/reset_touch

Clears the stored touch calibration and starts the calibration screen on the
device. The response returns immediately. The user then touches the targets on
the screen.

- Side effect: the touch calibration values are erased from flash, and the
  device enters calibration (the same flow as holding the screen at boot).
- Response `200`: `{"status":"ok"}`.
- Errors: `415` / `403` as above.

```sh
curl -s -X POST http://BOARD_IP/api/reset_touch -H 'Content-Type: application/json' -d '{}'
```

## GET /api/status

Runtime information.

Response `200`:

```json
{
  "ip": "192.0.2.10",
  "rssi": -58,
  "uptime_s": 1234,
  "heap_free": 150000,
  "version": "2.0.0-dev",
  "qrz_status": "disabled",
  "uart2_rx_pin": 27,
  "uart2_tx_pin": 22
}
```

- `ip` is the STA address, or the AP address (`192.168.4.1`) in AP mode.
- `qrz_status` is one of `disabled`, `ok`, `error: login http`,
  `error: login failed`, `error: lookup http`, `error: session` (QRZ lookups
  report their last state).
- The pin numbers are for the `cyd` board (`esp32-38pin` uses 16 and 17).

```sh
curl -s http://BOARD_IP/api/status
```

## GET /api/uart_status

State of the UART2 connected module.

Response `200`:

```json
{
  "uart2_connected_module": "none",
  "bt_active": false,
  "aprs_uart_active": false
}
```

- `bt_active` is true when the UART2 bridge mode is active.
- `aprs_uart_active` is true when the module is `aprs_tnc`.

## GET /api/bridge_bt_status

State of the ESP32 Bluetooth bridge (used when `uart2_connected_module` is
`esp32_bridge`).

Response `200` (bridge enabled):

```json
{
  "enabled": true,
  "scan_in_progress": false,
  "last_error": "",
  "last_ack": "",
  "disc_status": "",
  "pair_pending": false,
  "pair_result": "none",
  "paired_name": "ExampleSpeaker",
  "paired_addr": "AA:BB:CC:DD:EE:FF",
  "devices": [
    {"name": "ExampleSpeaker", "addr_raw": "AABBCCDDEEFF", "addr": "AA:BB:CC:DD:EE:FF"}
  ]
}
```

- `pair_result` is `ok`, `err` or `none`.
- When the bridge is not active, the response may contain only `{"enabled":false}`.

```sh
curl -s http://BOARD_IP/api/bridge_bt_status
```

## POST /api/bridge_bt_scan

Starts a Bluetooth device scan. Results appear in `/api/bridge_bt_status`.

- Requires bridge mode. Otherwise `400 {"status":"error","message":"bridge_mode_required"}`.
- `503 {"status":"error","message":"busy"}` if the bridge is busy.
- Success `200`: `{"status":"ok","action":"scan_started"}`.
- The body is ignored (send `{}`), but `Content-Type: application/json` is required.

```sh
curl -s -X POST http://BOARD_IP/api/bridge_bt_scan -H 'Content-Type: application/json' -d '{}'
```

## POST /api/bridge_bt_pair

Starts pairing with a device found by a scan.

Request body:

```json
{"name": "ExampleSpeaker", "address": "AA:BB:CC:DD:EE:FF", "password": "1234"}
```

- `password` is optional and at most 16 characters.
- Errors:
  - `400 {"status":"error","message":"bridge_mode_required"}`
  - `400 {"status":"error","message":"json_parse"}`
  - `400 {"status":"error","message":"password_too_long"}`
  - `400 {"status":"error","message":"pair_start_failed"}`
- Success `200`: `{"status":"ok","action":"pair_started"}`. Poll
  `/api/bridge_bt_status` for `pair_result`.

```sh
curl -s -X POST http://BOARD_IP/api/bridge_bt_pair \
  -H 'Content-Type: application/json' \
  -d '{"name":"ExampleSpeaker","address":"AA:BB:CC:DD:EE:FF","password":""}'
```

## Access point mode

When the board cannot join the configured WiFi network, it starts its own
access point:

- SSID: `ESP32-HAM-CLOCK`
- Password: the `ap_pass` setting (default `1234567890`). If `ap_pass` is
  shorter than 8 characters, the AP starts **open** (the web page refuses to
  save a shorter password).
- Address: `http://192.168.4.1`

All routes work the same in AP mode.

```sh
curl -s http://192.168.4.1/api/status
```

## Backup and restore to a new clock

1. On the old clock, open the configuration page and press **Export backup**,
   or run `curl -s -o esp32-ham-clock-backup.json http://BOARD_IP/api/backup`.
   The file contains every secret in plain text. Keep it private.
2. Flash the firmware on the new clock and join it to the AP
   (`ESP32-HAM-CLOCK`, default password `1234567890`), or open its LAN address.
3. On the new clock, open the configuration page, press **Import backup** and
   select the file. Then press **Save**. The page sends only the fields that
   differ from the device, and secret fields are sent with their values from
   the file.
4. If WiFi changed, the board restarts after the save. Check
   `/api/status` for the new IP.
5. Delete the backup file when it is no longer needed.

The same restore with curl. The backup file is posted to `/api/save` as is:

```sh
curl -s -o esp32-ham-clock-backup.json http://BOARD_IP/api/backup
curl -s -X POST http://NEW_BOARD_IP/api/save \
  -H 'Content-Type: application/json' \
  --data-binary @esp32-ham-clock-backup.json
```

Notes:

- The backup has the same keys as `/api/config`, with the secret values in
  plain text. `user_lat_lon_valid` is ignored on restore.
- An empty secret value keeps the board's current secret. A `/api/config`
  response (which has only `*_set` flags) therefore does not restore secrets.
- The touch calibration of the old panel (`tcal`) is restored too and applied
  at once. If touch is off on the new board, recalibrate (`/api/reset_touch`,
  a 15 s hold on the clock screen, or Settings).
- If the restore changes WiFi or UART2 settings, the new board restarts; check
  `/api/status` afterwards.

## Configuration field reference

Values come from `kFields` (`lib/core/config_codec.cpp`) and `configDefaults()`
(same file), plus the `configStoreLoad` overrides. Defaults are the values on a
freshly erased board. Ranges are the clamp limits (`min`..`max`). "Restart"
means the board reboots after a save that changes the field.

Types: `str` (string), `int`, `float`, `bool`, `lang` (`en` / `pt-br`),
`int[12]` (array of integers).

Sections follow the web page.

### WiFi

| JSON key | Type | Default | Range / limits | Secret | Restart |
|---|---|---|---|---|---|
| `wifi_ssid` | str | empty | max 32 bytes | no | yes |
| `wifi_pass` | str | empty | max 64 chars | **yes** | yes |
| `wifi_ssid2` | str | empty | max 32 bytes | no | yes |
| `wifi_pass2` | str | empty | max 64 chars | **yes** | yes |
| `ap_pass` | str | `1234567890` | max 64 chars; under 8 chars = open AP | **yes** | yes |

### Station

| JSON key | Type | Default | Range / limits | Secret | Restart |
|---|---|---|---|---|---|
| `user_callsign` | str | empty | max 15 chars | no | no |
| `user_locator` | str | empty | max 10 chars | no | no |
| `user_lat` | float | 0 | -90 .. 90 | no | no |
| `user_lon` | float | 0 | -180 .. 180 | no | no |
| `user_lat_lon_valid` | bool | false | read-only (ignored on save); reported true when lat or lon is non-zero | no | no |
| `timezone` | float | 0 | -12 .. 14 (hours) | no | no |

### DX Cluster

| JSON key | Type | Default | Range / limits | Secret | Restart |
|---|---|---|---|---|---|
| `cluster_host` | str | `dxcluster.iu1bow.it` | max 63 chars | no | no |
| `cluster_port` | int | 7300 | 1 .. 65535 | no | no |
| `cluster_noann` | bool | true | | no | no |
| `cluster_nowwv` | bool | true | | no | no |
| `cluster_nowcy` | bool | true | | no | no |
| `cluster_usefilters` | bool | true | | no | no |
| `cluster_filters` | str | `set/ft8` | max 127 chars, newlines kept | no | no |
| `pota_api_url` | str | `https://api.pota.app/v1/spots` | max 95 chars | no | no |

### HamAlert

| JSON key | Type | Default | Range / limits | Secret | Restart |
|---|---|---|---|---|---|
| `hamalert_host` | str | `hamalert.org` | max 63 chars | no | no |
| `hamalert_port` | int | 7300 | 1 .. 65535 | no | no |
| `hamalert_login` | str | empty | max 31 chars | no | no |
| `hamalert_password` | str | empty | max 31 chars | **yes** | no |

### APRS-IS

| JSON key | Type | Default | Range / limits | Secret | Restart |
|---|---|---|---|---|---|
| `aprs_host` | str | `rotate.aprs2.net` | max 63 chars | no | no |
| `aprs_port` | int | 14580 | 1 .. 65535 | no | no |
| `aprs_callsign` | str | `nocall` | max 11 chars | no | no |
| `aprs_passcode` | str | `0` | max 7 chars | **yes** | no |
| `aprs_ssid` | int | 0 | 0 .. 15 | no | no |
| `aprs_radius` | int | 10 | 1 .. 150 (km) | no | no |
| `aprs_beacon` | bool | true | | no | no |
| `aprs_symbol` | str | `/V` | max 2 chars | no | no |
| `aprs_comment` | str | empty | max 43 chars | no | no |
| `aprs_interval_min` | int | 29 | 1 .. 180 (min) | no | no |

### APRS alerts

| JSON key | Type | Default | Range / limits | Secret | Restart |
|---|---|---|---|---|---|
| `aprs_alert` | str | empty | max 127 chars (CSV watch list) | no | no |
| `aprs_alert_enabled` | bool | true | | no | no |
| `aprs_alert_nearby_enabled` | bool | true | | no | no |
| `aprs_alert_wx_enabled` | bool | false | | no | no |
| `aprs_alert_min_sec` | int | 300 | 0 .. 86400 (s) | no | no |
| `aprs_alert_screen_sec` | int | 60 | 1 .. 60 (s) | no | no |
| `aprs_alert_distance_km` | float | 1.0 | 0.1 .. 500 (km) | no | no |

### Weather

| JSON key | Type | Default | Range / limits | Secret | Restart |
|---|---|---|---|---|---|
| `weather_key` | str | empty | max 39 chars (OpenWeather API key) | **yes** | no |
| `weather_alert_storm_enabled` | bool | false | | no | no |
| `weather_alert_wind_enabled` | bool | false | | no | no |
| `weather_alert_wind_threshold_ms` | float | 12.0 | 1 .. 60 (m/s) | no | no |

### LED and QRZ

| JSON key | Type | Default | Range / limits | Secret | Restart |
|---|---|---|---|---|---|
| `enable_led_alert` | bool | true | | no | no |
| `led_alert_duration_ms` | int | 5000 | 100 .. 60000 | no | no |
| `led_alert_blink_ms` | int | 500 | 50 .. 5000 | no | no |
| `qrz_user` | str | empty | max 31 chars | no | no |
| `qrz_pass` | str | empty | max 31 chars | **yes** | no |

### UART2 / bridge

| JSON key | Type | Default | Range / limits | Secret | Restart |
|---|---|---|---|---|---|
| `uart2_connected_module` | str | `none` | max 19 chars; page offers `none`, `aprs_tnc`, `aprs_tnc_kiss`, `esp32_bridge`; a stored `gps` value is converted to `none` | no | yes |
| `uart2_baud` | int | 9600 | 300 .. 2000000 | no | yes |
| `uart2_data_bits` | int | 8 | 7 .. 8 | no | yes |
| `uart2_parity` | str | `none` | max 5 chars; page offers `none`, `even`, `odd` | no | yes |
| `uart2_stop_bits` | int | 1 | 1 .. 2 | no | yes |

### Display

| JSON key | Type | Default | Range / limits | Secret | Restart |
|---|---|---|---|---|---|
| `tft_backlight` | int | 100 | 5 .. 100 (%) | no | no |
| `tft_night_dimming` | int | 0 | 0 .. 100 (%) | no | no |
| `tft_invert` | bool | true (`DISPLAY_INVERT_DEFAULT`) | | no | no |
| `tft_lang` | lang | `en` | `en` or `pt-br` (other values rejected) | no | no |
| `tft_rotation` | int | 1 | 1 or 3 (other values become 1) | no | no |
| `tft_auto_switch` | bool | false | | no | no |
| `tft_switch_time_sec` | int | 30 | 1 .. 3600 (s) | no | no |
| `screen_order` | int[12] | `[1,2,6,9,4,3,5,12,7,8,0,0]` | each 0 .. 12 (10 and 11 are the Open-Meteo screens); 0 = off; invalid or duplicate ids become 0 | no | no |
| `scr_en` | int | 5118 (`0x13FE`) | bitmask, clamped to 0 .. 16382 (`0x3FFE`), bits outside `0x1FFE` always cleared | no | no |

`scr_en` bit = screen id: 1 HamClock, 2 DX Cluster, 3 Sunspots, 4 Band info,
5 Weather, 6 APRS list, 7 POTA, 8 HamAlert, 9 APRS radar, 10 Weather - Open-Meteo, 11 Forecast - Open-Meteo, 12 Forecast.
The Open-Meteo screens are off by default (they are not in the default mask). An
enabled one missing from `screen_order` is appended to the first free slot.

### Keys without a form field

These are stored in NVS and are in `/api/config` and the backup. The page does not
edit them (they are set from the device), but Import restores them:

- `screen1_time`, `s2_mode`, `s2_band`, `s7_mode`, `s7_band`, `s8_mode`,
  `s8_band`, `aprs_sort` (screen filters and sort order, ints)
- `br_pair_addr`, `br_pair_name` (Bluetooth bridge pairing)
- `tcal`: touch calibration, `[a,b,c,d,e,f]` (x = a*rx + b*ry + c, y = d*rx +
  e*ry + f). Omitted while the panel is uncalibrated. On write it must be 6
  finite numbers with |a|,|b|,|d|,|e| <= 1, |c|,|f| <= 4096 and a non-zero
  determinant, else `400 bad_value:tcal` and nothing is changed. It takes effect
  at once, without a reboot or the calibration screen. `/api/reset_touch` clears it.
