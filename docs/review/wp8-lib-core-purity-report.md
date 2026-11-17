# WP8 report: lib/core purity and tests (A-04, A-07, N-03, M-03, L-05)

Only `lib/core`, `test/` and `src/ui/screens/registry.cpp` (A-04 static_asserts) were changed. Call sites owned by other
work packages were NOT edited; the migration list below is for the call-site owners or the final integration pass.

## Done
- `RingBuffer<T,N>` is circular (head index, O(1) `pushFront`); API and newest-first semantics unchanged.
  `test_ring_buffer`: wrap (40 pushes into N=4), capacity 1, clear+reuse, `moveToFrontIf` at head/middle/tail on a wrapped buffer, first match only.
- Dead fields removed from `lib/core/types.h`: `Spot.spotter`; `WeatherData.forecast3hDesc/forecastNextDayDesc/forecastError/lastError/updated`,
  `forecast3h{TempC,WindMs,Valid}`, `forecastNextDay{TempC,WindMs,Valid}`. Writers in dx_parse/pota_parse/hamalert_parse/weather_json and the
  matching test assertions were removed. `WeatherData` shrinks by ~240 B (the `.bss` store copy: 93625 -> 93401 B).
- A-04: `static_assert` block in `src/ui/screens/registry.cpp` ties every `scr::X` to `ScreenId::X`.
- Not removed (still written by call sites owned by other WPs): `Spot.lat/lon/hasLatLon` (written in `src/svc/qrz.cpp:74`,
  `src/svc/dx_cluster.cpp:71`, `src/svc/hamalert.cpp:95`; no reader anywhere). Remove them once those three lines are dropped (-9 B per Spot).
  Also `src/svc/pota.cpp:30` still lists `"spotter"` in its JSON filter; drop that entry.

## Moved functions: header, signature, call sites to switch
| New module (lib/core) | Signature | Replaces / call sites |
|---|---|---|
| `url_encode.h` | `void urlEncode(const char* in, char* out, size_t cap)` | delete the static in `src/svc/qrz.cpp:39`, `#include "url_encode.h"` (2 calls in `login()`) |
| `gps_time.h` | `int64_t daysFromCivil(int y,int m,int d)`; `bool gpsToUnix(uint32_t ddmmyy, uint32_t hhmmss, int64_t& unix)` | `src/svc/uart2.cpp:58-77`: delete `daysFromCivil`; in `syncClock` keep the fix/60 s/NTP guards, then `int64_t t; if (!gpsToUnix(g.date, g.utc, t)) return; struct timeval tv = {(time_t)t, 0};` (the date/time field validation moved into `gpsToUnix`) |
| `aprs_symbol.h` | `enum class AprsIcon {None,Car,House,Human}`; `bool aprsSymbolName(const char sym[2], Tr& out)`; `void aprsSymbolInfo(const char sym[2], AprsIcon& icon, Tr& desc)` | `symName` in `src/ui/screens/aprs_list_screen.cpp:30` -> `Tr n; if (aprsSymbolName(a.sym, n)) ... tr(n)`; `symbolInfo` + `IconKind` in `src/ui/overlays/alert_overlay.cpp:33,145` -> `aprsSymbolInfo` (map `AprsIcon::Car/House/Human` to the existing `ICON_*` or replace the enum). One table now feeds both. |
| `wx_map.h` | `enum class WxIcon {None,I200,I300,I500,I502,I511,I520,I600,I611,I700,I800,I800n,I801,I801n,I803}`; `WxIcon wxIconForId(int id, bool night)` | `src/ui/icons/weather_icons.cpp`: keep `weatherIcon(id, night)` but reduce it to `switch (wxIconForId(id, night))` returning `&wxicon_<n>` (nullptr for `None`) |
| `wx_map.h` | `int wxWindLevel(float ms)` (0..3: <5, <10, <15, >=15) | `wxWindColor` in `src/ui/screens/wx_common.cpp:55`: level -> `COL_MODE, COL_FREQ, 0xFF7F00, COL_ERROR` |
| `wx_map.h` | `int wxPmLevel(float v,float a,float b,float c,float d)` (0..4) | `pmColor` in `src/ui/screens/weather_screen.cpp:19`: level -> `COL_MODE, COL_FREQ, 0xFF7F00, COL_ERROR, 0xF80000` |
| `prop_level.h` | `enum class PropCond {Unknown,Good,Fair,Poor}`; `PropCond propCondOf(const char*)` | `propCondColor` in `src/ui/screens/prop_common.h:12`: Good 0x00FF00, Fair 0xFFFF00, Poor 0xFF0000, Unknown 0xC0C0C0 |
| `prop_level.h` | `enum class KLevel {None,Low,Mid,High}`; `KLevel kIndexLevel(const char*)` | `PropagationScreen::kColor` in `src/ui/screens/propagation_screen.cpp:84` (keep the `!d_.valid` check): None COL_TEXT, Low 0x00FF00, Mid 0xFF8C00, High 0xFF0000 |

Behaviour is identical to the originals (same thresholds, same 7-char uppercase buffer in `propCondOf`).
Each new header has `test/test_url_encode`, `test_gps_time`, `test_aprs_symbol`, `test_wx_map`, `test_prop_level`.

## Numbers (cyd)
- Flash text 1,426,713 -> 1,425,709 B; .bss 93,625 -> 93,401 B; heap per `Spot` -0 (lat/lon kept), -12 B (`spotter`) x 160 spots = -1.9 KB.
- `pio test -e native`: 159 cases pass. `pio run -e cyd -e esp32-38pin`: build OK.

## Hardware checks still needed
None specific (pure refactor). Optional: confirm DX list order after repeated spots (ring buffer wrap) on device.
