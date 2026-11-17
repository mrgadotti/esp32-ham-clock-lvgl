# WP2 report: UI manager and OOM safety

## Changes
- `ui_manager.cpp` `showForce` (C-02): the old screen (and boot screen) is deleted BEFORE the new one is created. A 1-object temporary screen stays active during the gap (never rendered: same thread, no `lv_timer_handler` in between) and is deleted after `lv_screen_load(new)`. LV_MEM peak is now max(old, new) instead of old + new.
- New log line right after `create()`: `[MEM] screen <id> create lvfree=<bytes>`. It is a single sample taken after `create()`, not a guaranteed minimum (transient dips inside `create()` are not sampled). Both `[MEM]` lines also print `big=` (largest free block). The old 3 s line is now `[MEM] screen <id> settled lvfree=...`.
- C-13: `memShowAt` compare is wrap-safe.
- C-11: `boot_screen.cpp` nulls its static `log` on `LV_EVENT_DELETE`.
- F-13: `PlaceholderScreen` removed; reserved Matrix/Game ids are normalised to HamClock in `uiRequest`/`show()`, and `createScreen` falls back to the clock screen (never null). `uiNext/uiPrev` removed.
- `lv_conf.h` (assert handler) not touched: owned by WP1.

## Numbers
- cyd: flash 1,786,337 -> 1,786,709 B (+372, the extra log), static RAM 118,260 -> 118,260 B. esp32-38pin: flash 1,786,169 B, RAM 118,236 B.
- `pio test -e native`: all pass. Both firmware builds succeed.
- `tools/check_text_fit.py` reports 5 overflows (e.g. APRS list UTC column); none of the touched files affect layout, so these are pre-existing (not verified against main by a rerun).

## Worst-case LV_MEM reasoning (40 KB pool)
Steady use per screen (review C-02): Settings 21.3 KB, full DX (100 rows) about 22 KB, Pota 15.7 KB, AprsList 11.4 KB. With the old screen freed first, free memory before `create()` is about the pool minus persistent objects, so the worst create (full DX or Settings) leaves about 18 KB free, above the 6 KB target for every ordered pair. Before the change, DX(22)+Settings(21.3) = 43 KB exceeded the pool.

## Hardware checks still needed
1. Cycle every ordered screen pair incl. a full DX list (100 rows) -> Settings; read `[MEM] screen N create lvfree=`: all >= 6 KB.
2. No visible flash or glitch at transitions (the tmp screen is never rendered, expected identical to before); swipe behaviour unchanged; language change rebuild (`pendingRebuild`) still works.
3. Forced OOM: after WP1 lands the assert handler, set `LV_MEM_SIZE` to 16K temporarily in `include/lv_conf.h`, flash, open Settings; expect a `[LV] assert` line and a reboot instead of a frozen UI. Revert afterwards.
