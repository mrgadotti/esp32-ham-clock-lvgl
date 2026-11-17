# WP3 report: Spot/APRS lists and tables

Branch: worktree-agent-ae18fb572eb8497d8

## Changes
- A-01: `SpotListScreen` serves DX, POTA and HamAlert (`SpotListDef` + type-erased `SpotSrc` over `Guarded<RingBuffer<Spot,N>>`, so no template instantiation per ring size). `dx_screen.cpp` is now a 9-line factory; keys `s2_*`, 12 band tiles incl. 30m, MSG column with comment (cut at 15 chars, UTF-8 safe), COUNTRY for POTA/HamAlert. `registry.cpp` untouched.
- M-04: `kDxSpots` named constant in `data_store.h` (see review fixes: now 75). First estimate at 100 rows: 500 cells x (8 B header + text + 1, 4-aligned, +4 TLSF) ~ 20-24 B + 2.4 KB pointer/row arrays = about 12 KB worst case, typically 11-12 KB (comment cut at 15 chars to keep it there). 50 would halve it (UX choice, not taken).
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

## Review fixes
- `RadarZoomReset` was double-encoded (31 C3 83 C2 97); now "1" + U+00D7 in both languages. `test_i18n` gained `test_no_mojibake` (no U+0080-U+009F, no C3 83 followed by C2) over all Tr strings in EN and PT-BR, plus an exact check of the x sign.
- Radar `ring()`: each chord is clip-tested against `layer->_clip_area` (onDraw runs once per render chunk, so only chords touching the 20-line strip become draw tasks), chord length 2*sqrt(r) (sagitta 0.5 px), angle advanced by a rotation (one sin/cos per ring, not per segment). Worst case per strip is about 12 tasks per big ring (a chord is 28 px at r=200, 56 px at r=800), so under 40 even with the alert ring; rings off screen are skipped. Not capped in code (a cap would leave gaps).
- DX ring: 100 -> 75 spots (`kDxSpots`, also used in `dx_cluster.cpp`, one-line change). Cost per spot with the pessimistic 8 B allocator overhead: cells 24+24+24+(20..24)+(28..32) B + 24 B row arrays = 144-152 B, so 75 rows = 10.8-11.4 KB (100 would be 14.4-15.2 KB). MSG stays at 15 chars: the 4-byte rounding makes a 12-char cut save nothing. Heap: -25 spots x about 140 B = -3.5 KB.
- `SPOT_LIST_MAX = kDxSpots`, `static_assert(N <= SPOT_LIST_MAX && N <= 255)`.
- `lib/core/text_cut.h/.cpp` (`textCut`, `formatKmCell`) with `test/test_text_cut` (5 cases); the spot and APRS lists use them.
- `check_text_fit.py`: WARN band (fits with <= 1 px slack; exit status unchanged) and a DX/POTA/HamAlert title check at x=46. Current WARNs: spot CALL 74/75, APRS CALL 92/93, SYM 84/84, FREQ 57/57, APRS sort tile 85/86, radar range label 60/60. Overflows left: forecast night column and clock date (WP4).
- Totals after the fixes: 145 native tests pass; cyd flash used 1774661 B, esp32-38pin 1774213 B, both under limits.
