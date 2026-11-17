# WP7 report: i18n, fonts, dead code

Findings: C-08, C-15 (touch cal + auto-switch option), F-05 (font part), F-06, F-07, F-12, F-14, A-05, A-09 (no change needed).

## Changes

- **Fonts (F-05/06/07).** `tools/gen_fonts.sh` regenerates all fonts with `--no-kerning`, range `0x20-0x7E,0xB0,0xC0-0xF6,0xF8-0xFF` (keeps every Latin-1 letter for weather text and city names, plus the degree sign and U+00D7 used by the radar zoom button; the report's range would have dropped U+00D7). The `.fallback = &lv_font_montserrat_14` sed is gone (fallback is NULL). `font_14` now also embeds the two symbols the theme draws, U+F00C (checkbox tick) and U+F078 (dropdown arrow), taken from the FontAwesome file inside the LVGL package, so nothing needs the built-in font any more. Tabular digits re-applied (adv_w 150/193/257/514 unchanged). font_18 line height went 23 -> 22 px (dropped tall punctuation glyphs).
- **i18n (C-08, F-12).** 36 dead keys removed; kept `Off` (used now) and `KmH` (for WP4's overlay unit). The four `Cal*` keys now hold the real calibration messages: `CalTouch`, `CalVerify`, `CalBad`, `CalFailed`, with PT-BR text. `touch_cal_ui.cpp` uses `tr()` and sets `font_14` on the message label (the default font has no accents). The auto-switch dropdown uses `tr(Tr::Off)`. Language names stay literal (endonyms).
- **F-14.** Weak definitions of `onConfigChanged`, `uart2SendKiss`, `qrzOnNewSpot` removed (strong ones exist; `aprs_beacon.cpp` now includes `svc/uart2.h`); "Task 11/17" comments removed.
- **A-05.** `touch_rot` removed from `Config`, `kFields`, defaults and the "Touch rotation" option in `web/config.html` (single-line edits). `uart2_mode` left to WP6c.
- **Dead code.** `src/ui/fonts_demo.*` deleted (unreferenced). A-09: reserved `Matrix/Game` ids kept; history docs left in place.
- `tools/check_text_fit.py`: dropped removed keys, added a row for the calibration prompts.

## Numbers (flash, `pio run -e cyd`)

| step | flash B | RAM B |
|---|---|---|
| before | 1,786,889 | 118,260 |
| this WP | 1,774,609 (-12.3 KB) | 118,236 |
| this WP + lv_conf snippet below (verified in a scratch edit, then reverted) | 1,760,945 (-25.9 KB) | 118,236 |

`esp32-38pin` builds (1,774,085 B). `pio test -e native`: 131/131 pass (test_i18n, test_config_codec included).
`tools/check_text_fit.py`: output identical to main except the removed Pota/HamAlert filter titles; the same 5 overflows exist on main already (APRS list UTC/CALL/FREQ cells, forecast night column, clock date `Wednesday, 30 September 2026`), owned by WP3/WP4.
Accent coverage script: every non-ASCII char in `i18n.h` and the sources (U+B0 C1 C3 C7 CD D3 D5 D7 E0 E1 E2 E3 E7 EA F3) is in font_14/18/24.

## For WP1 (include/lv_conf.h, not edited here)

```
#define LV_FONT_MONTSERRAT_14 0                                  /* line 489 */
#define LV_FONT_CUSTOM_DECLARE LV_FONT_DECLARE(font_14)          /* line 521 */
#define LV_FONT_DEFAULT &font_14                                 /* line 524 */
```
Keep `LV_USE_FONT_PLACEHOLDER 1`. Without these lines the built-in font stays linked (only via `LV_FONT_DEFAULT`).

## Hardware checks still needed

- Dropdown arrow and checkbox tick still show (font_14 symbols), PT-BR accents in dropdown popup and calibration prompts, text overlap on screens with font_18 (1 px shorter line), no tabular-digit jitter on clock/values.
