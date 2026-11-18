# APRS Symbol Icons Implementation Plan

**Goal:** Show the APRS symbol as a drawn icon instead of its text name in the APRS alert overlay, add an APRS beaconing overlay icon, and draw the symbol icon in place of the dot on the APRS radar. The APRS-IS list is out of scope (no icon column, no row height change).

**Architecture:** Reuse the vector drawing already in `src/ui/overlays/alert_overlay.cpp` (`onIconDraw`, Car / House / Human). Move it to a shared widget so the overlay and the list draw the same shapes. No bitmap assets, no asset pipeline. The beacon event carries the TX symbol so the overlay can pick the same icon.

**Tech Stack:** PlatformIO espressif32@6.12.0, Arduino, LVGL 9.6, Unity native tests (`pio test -e native`).

## Flash cost (measured on the current `cyd` build)

| Item | Flash |
|---|---|
| Firmware today (`.pio/build/cyd/firmware.bin`) | 1 484 016 B |
| Existing vector icons, `onIconDraw` (Car, House, Human) | 983 B code |
| `aprsSymbolInfo` + symbol table | ~250 B |
| One 50x50 RGB565 bitmap (weather style) | 5 000 B |
| One 16x16 RGB565 bitmap | 512 B |
| One 16x16 alpha-only bitmap (A1 or A8 with colour from draw) | 32 B (A1) / 256 B (A8) |

Conclusion: flash is not the constraint. The app uses about 1.5 MB of a ~3 MB app partition, so every option below costs under 2 % of flash.

| Option | Extra flash | Notes |
|---|---|---|
| **A. Reuse vector icons, scaled (recommended)** | ~0.2–0.5 KB | Same shapes, no assets. Only 3 symbols have an icon, so the rest keep the text fallback. |
| B. 16x16 RGB565 bitmaps for the 3 icons | ~1.5 KB | Crisper at 16 px, but a new generator step and four more files. |
| C. Vector icons for all ~30 table symbols | ~5–10 KB | Most of the cost is drawing code. Only worth it if the text fallback is unwanted. |
| D. Weather-style 50x50 bitmaps | 15 KB for 3 icons | Too big for a 16 px row. Not recommended. |

## Scope

- Alert overlay (`EventKind::AprsAlert`): already draws the icon for Car/House/Human. Keep it. The text fallback for other symbols stays as is.
- Beaconing overlay (`EventKind::Beaconing`): today it shows only call, position and path. Add the icon for the configured `aprs_symbol`, with the text fallback.
- APRS radar (`AprsRadarScreen`): each station is a 3 px dot in `stationColor`, with the callsign label. Draw the icon (scaled, ~12–16 px) for Car/House/Human, keep the dot for other symbols, and move the label clear of the icon.
- APRS-IS list (`AprsListScreen`): out of scope. No icon column and no row height change.

## Tasks

- [ ] **1. Shared icon drawing.** Move `onIconDraw`'s shapes out of `alert_overlay.cpp` into `src/ui/widgets/aprs_icon.{h,cpp}`: `void drawAprsIcon(lv_layer_t*, const lv_area_t&, AprsIcon, float scale)`. Scale every coordinate, so one set of shapes serves the 64x42 overlay and a ~18 px list cell. Keep the overlay output identical at scale 1.
- [ ] **2. Beacon carries the symbol.** Add `char sym[2]` to `BeaconInfo` (`lib/core/types.h`). Fill it in `src/svc/aprs_beacon.cpp` from the `sym` it already builds. Read it in `alert_overlay.cpp` `show()` for `EventKind::Beaconing`: draw the icon, or use the `tr(desc)` text when `AprsIcon::None`. Layout: icon left, call/position/path right, as in `buildAprs`.
- [ ] **3. Radar icons.** In `onDraw` (`aprs_radar_screen.cpp`), replace `arc(l, x, y, 3, 3, t.col)` with `drawAprsIcon` at ~14 px for Car/House/Human; keep the dot for `AprsIcon::None`. Move the label up by the icon's half height (`y - 18` becomes about `y - 22`) so the label check still works. Check how the icon and label overlap at zoom 1, where 30 stations fit in 200 px.
- [ ] **4. Tests.** Add Unity cases to `test/test_aprs_symbol` if a helper maps symbol to icon (the mapping itself does not change). Run `pio test -e native`. Build `pio run -e cyd` and compare size with 1 484 016 B. Expect a delta under 1 KB.
- [ ] **5. Screen check.** Run `tools/screen_test.py` (`cyd_test` env) for the alert, beacon and radar screens (radar at zoom 1 and 4), in PT and EN. Icons drawn at ~18 px lose the 2 px antenna and wheel details, so check them on the screenshot.

## Risks and open questions

- The beacon overlay currently shows no symbol name. The plan adds the icon, not a name, which matches "instead of the name" only for the alert overlay. Confirm this reading.
- Radar: at zoom 1, stations cluster near the QTH and icons can overlap each other and the labels. The radar already hides labels when there are more than 11 stations at zoom 1 (`labels`), so icons may need the same rule.
- Vector shapes scaled to ~14 px may look muddy. If so, step B (16x16 bitmaps, about 1.5 KB) is the fallback.
