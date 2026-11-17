# WP3 report: Spot/APRS lists and tables

Branch: worktree-agent-ae18fb572eb8497d8

## Changes
- A-01: `SpotListScreen` serves DX, POTA and HamAlert (`SpotListDef` + type-erased `SpotSrc` over `Guarded<RingBuffer<Spot,N>>`, so no template instantiation per ring size). `dx_screen.cpp` is now a 9-line factory; keys `s2_*`, 12 band tiles incl. 30m, MSG column with comment (cut at 15 chars, UTF-8 safe), COUNTRY for POTA/HamAlert. `registry.cpp` untouched.
- M-04: `kDxSpots` named constant in `data_store.h` (still 100). DX table cost estimate at 100 rows: 500 cells x (8 B header + text + 1, 4-aligned, +4 TLSF) ~ 20-24 B + 2.4 KB pointer/row arrays = about 12 KB worst case, typically 11-12 KB (comment cut at 15 chars to keep it there). 50 would halve it (UX choice, not taken).
- M-08: table refresh at most 1 Hz when the store version changed (immediately after a filter/sort save); `TEXT_CROP` set only on newly created rows; unchanged cell text is not rewritten. No rebuild when the version is unchanged (as before).
- M-05 (APRS list): no `new AprsStation[30]`; index snapshot and insertion sort run under the store lock on the member `idx_`.
- C-15 (radar): new keys `RadarNorth`, `RadarZoomReset` in `lib/core/i18n.h` (WP7 owns the file: append-only change next to the APRS symbol block).
- UI kit `src/ui/widgets/ui_kit.h/.cpp`: `uiLabel`, `uiTile`, `uiTilePaint`, `uiHamburger`, `uiModalPanel` (the UTC clock helper is `headerTickUtc` in header.h, now used by all list screens). `spot_table` adds `spotListUiCreate/Set` (header + column titles + table + waiting label), shared by the spot and APRS lists. WP4 can use the kit for the remaining label/tile code (bands, propagation, clock, alert overlay, settings).
- Coordinator findings: hamburger bars no longer clickable and the button is 50x26 (bars swallowed taps); waiting/no-login label wraps in 300 px; UTC column 40 px (gap to CALL >= 3 px); radar `N` moved left of the km labels (optional, skipped when overlapping).
- Radar freeze at 8x, root cause found in the LVGL source: `lv_draw_arc` builds a circle mask of about 22 B per pixel of radius (800 px rings at 8x), a failed `lv_malloc` hits `LV_ASSERT_MALLOC` and `LV_ASSERT_HANDLER` is `while(1)`. Rings with radius > 128 px are now drawn as 12 px line segments, only where they cross the screen; rings that miss the screen are skipped.

## check_text_fit.py
The 5 pre-existing overflows: the 3 APRS list ones (UTC, CALL, FREQ cells; cause: the script assumed 3 px padding, the table used 2+2) are fixed; table padding is now left 1 / right 0 (`SPOT_TABLE_PAD_L`), APRS columns 40/94/85/43/58, spot columns 40/76/60/42/102, KM >= 10000 shows as `12k`. Remaining 2 are outside WP3 (forecast night column, clock date, WP4). Added rows for the spot list (cells, headers, wrapped waiting labels).

## Numbers
- cyd flash image 1781776 -> 1780912 B (-0.9 KB; text -932, data +72; the ui_kit, ring helper and wrap code eat part of the A-01 saving), static RAM 93625 -> 93609 B bss. Flash used 1774341 B (56.4%), RAM 119604 B.
- esp32-38pin builds (flash 1773889 B). `pio test -e native`: 138/138 pass.

## Hardware checks still needed
- Tap on the hamburger icon at (20,13) and (35,13) on DX/POTA/HamAlert/APRS list; filter menu on all three.
- PT HamAlert no-login text (2 lines), UTC/CALL gap, APRS list columns, radar N vs `20.0km`, radar zoom 8x x3 with a configured alert distance (no freeze, frame time), LV_MEM headroom with 100 DX spots.
- DX spot arrival: table updates at most once per second.
