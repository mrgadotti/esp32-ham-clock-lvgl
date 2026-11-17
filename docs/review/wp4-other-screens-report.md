# WP4 report: other screens on the UI kit (A-02, K-03, N-05, C-09, C-15, F-13)

## Done
- A-02: `uiLabel` (ui_kit) gained `w` and `align`; weather, forecast, propagation, bands, location, clock, settings and
  the alert overlay use it. Removed: `wxLabel`, overlay `mkLabel`, clock `lbl`, bands `cell`, the repeated label blocks.
  `PropFeed` in `prop_common.h` holds the propagation/bands version + 30 s age refresh (was duplicated).
  `header.cpp`, `boot_screen.cpp` needed no helper changes (title/clock already shared via `headerTickUtc`).
- WP8 call-site migration (local copies deleted in the same commit): `wx_map.h` (`wxIconForId`, `wxWindLevel`, `wxPmLevel`
  in weather_icons.cpp, wx_common.cpp, weather_screen.cpp; new `wxLevelColor`), `prop_level.h` (`propCondOf`, `kIndexLevel`
  in prop_common.h, propagation_screen.cpp), `aprs_symbol.h` (alert_overlay.cpp `symbolInfo`/`IconKind` and
  aprs_list_screen.cpp `symName`).
- Settings and Location: no more `pend*` globals; values go to `cfgUpdate` through capturing lambdas. The dropdown list
  font (`font_14` on `lv_dropdown_get_list`) is kept.
- K-03: the 300 B `WeatherData` copies in weather/forecast refresh are members, the 256 B location text buffer is a member,
  overlayShow's title/body (200 B) is a pointer + a static 160 B buffer. Prologue frame sizes (objdump `entry`):
  WeatherScreen refresh 496 -> tick 224, ForecastScreen refresh 512 -> tick 240, LocationScreen refresh 384 -> tick 144,
  overlayShow 384 -> 192. ClockScreen tick 128 -> 144 (date buffer). The UI task watermark itself must be measured on the board.
- N-05: the per-second time label is 216 px (text 214 px, LONG_CLIP) at x=52 instead of 320 px; the date is a 296 px
  LONG_CLIP label and, with the mode label, is only set at midnight or when the UTC/local mode flips (was every second).
- C-09: `appendf` in alert_overlay.cpp clamps every append to the buffer.
- C-15: overlay "hPa" and "km/h" use `Tr::HPa`/`Tr::KmH`, location "km/h" uses `Tr::KmH`, boot "OK" uses `Tr::Ok`.
  Left as is on purpose: proper nouns/technical tags (ESP32-HAM-CLOCK, WiFi, NTP, SSID, IP, RSSI, PM2.5, PM10, "SWL",
  "English/Português" dropdown endonyms), the close glyph "X", the unit symbol "C", "D+n" fallback header, "--".
- F-13 (rest): removed `localTimeOf`, `memLogLvFree`; `qrzEnqueue` is now file-local (header declaration dropped).
- text-fit fixes (`tools/check_text_fit.py`, now 0 overflow, 7 tight):
  - forecast night column: `NIGHT -12` (78 px) overflowed 77 px; the degree sign is dropped (72 px; the temperature row
    above carries it). Script row follows.
  - clock date: `Wednesday, 30 September 2026` was 300 px in a 296 px label; a date longer than 27 bytes now uses the
    3-letter weekday (`Wed, 30 September 2026`, 235 px). The longest remaining date is 295/296 (Wednesday, 30 November).
    Script models the same rule. Clock time row now checks the 216 px label.
- Overlay text review (both languages): all APRS/beacon/weather overlay rows pass. Rows added/updated for the real beacon
  strings (`-90.0000 -180.0000`, `APRS,WIDE1-1,WIDE2-1`), APRS bearing/distance boxes, boot "WiFi: OK"/"NTP: OK".
  Vertical review of buildAprs: call y=52, freq y=82, first wx/icon/description line y=106..116, pressure y=142, wind
  y=160, comment y=176..214 inside the 44..234 frame; title y=8 clears the X button (x>=272, y<=40); beacon/weather title
  y=42 clears it; weather body (5 lines at y=76, 226 px) ends near y=186. No change needed.

## Numbers (cyd, `size`)
| | text | data | bss | pio Flash used |
|---|---|---|---|---|
| before | 1,389,385 | 370,304 | 90,481 | 1,737,365 |
| after | 1,391,929 | 370,400 | 90,617 | 1,740,005 |

Flash +2.6 KB, static RAM +136 B (static overlay body 160 B). The three WP8 pure modules were unlinked before and are now
reachable (about +0.8 KB with the symbol table); removing the local copies and duplicated label code gave back about
2.5 KB, but the `uiLabel` call sites, `PropFeed` instantiations and the date rule add it back. esp32-38pin builds too.
`pio test -e native`: 170 cases pass.

## Hardware checks still needed
- Screenshots of clock (time/date centred as before, midnight rollover, UTC/LOCAL toggle), forecast night column,
  weather, propagation/bands, location and the three overlays; compare with the previous screenshots.
- UI task stack watermark (`[MEM] uistk`) >= 3.2 KB free after cycling every screen and the overlays.
- Wednesday + September (or November) date on the clock in EN to see the 3-letter weekday fallback.
