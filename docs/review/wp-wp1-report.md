# WP1 report: build and LVGL config

Flash is the `pio run -e cyd` app size (esp32-38pin is within 500 B; `firmware.bin` is 1,498,208 B). Static RAM stayed 118.1-118.2 KB in every step.

| step | flash B | delta | cumulative |
|---|---|---|---|
| main, -O2 | 1,786,889 | | |
| -Os + `-O2` hybrid (`tools/perf_flags.py`), gnu++17, pins | 1,582,953 | -203,936 | -203,936 |
| + 26 unused LVGL options off | 1,530,293 | -52,660 | -256,596 |
| + draw formats AL88, I1, L8, A8, RGB888, XRGB8888 off | 1,490,061 | -40,232 | -296,828 |
| lv_conf.h regenerated for 9.6 (+ARCLABEL off, 8 KB layer buf, assert handler) | 1,491,041 | +980 | -295,848 |
| + `esp_rom_uart_tx_wait_idle(0)` in the assert handler | 1,491,633 | +592 | -295,256 |

Gates: <= 1.60 MB with the hybrid (1.583 MB), <= 1.50 MB with widgets/formats off (1.492 MB). `PLATFORMIO_BUILD_FLAGS=-DPERF_TEST pio run -e cyd` builds (1,492,357 B). `pio test -e native`: 131/131.

## What changed
- F-01: `-Os` globally (removed `build_unflags = -Os` and `-O2`). `tools/perf_flags.py` (pre: extra script, `AddBuildMiddleware` in the two-argument form so it reaches library envs) re-adds `-O2` to `lv_draw_sw_blend.c`, `lv_draw_sw_blend_to_rgb565.c`, `lv_draw_sw_fill/border/letter/img.c`, `lv_draw_sw_mask*.c`, `lv_refr.c` and `TFT_eSPI.cpp`. The match runs on a normpath'd, forward-slash path so it also works on Windows. The hybrid costs only +5 KB over pure -Os (the report estimated 10-15 KB). Verified with `pio run -v` that those objects get `-Os ... -O2` and the rest stay `-Os`.
- F-02/F-03: options turned off as listed in the report, plus ARCLABEL (it defaulted to 1 in 9.6).
- F-15: `include/lv_conf.h` rewritten for LVGL 9.6 (about 100 lines instead of 1,129), listing only options that differ from `lv_conf_internal.h`. Checked by dumping every `LV_*` macro with `gcc -E -dM` before and after: the only differences are the intended ones. Removes the `LV_COLOR_DEPTH` and `LV_ASSERT_HANDLER_INCLUDE` deprecation warnings. Kept `LV_OBJ_STYLE_CACHE 0` and `LV_BUILD_EXAMPLES 0` (the 9.6 defaults are 1).
- F-17: `ArduinoJson@7.4.3` (what was resolved), XPT2046_Touchscreen pinned to commit f956c5d (v1.4, what was resolved), `-std=gnu++17` with `build_unflags = -std=gnu++11`.
- M-07: `LV_DRAW_LAYER_SIMPLE_BUF_SIZE` 8 KB; `LV_DRAW_THREAD_STACK_SIZE` removed (no OS).
- C-02 (assert part): `LV_ASSERT_HANDLER { esp_rom_printf("[LV] assert\n"); esp_restart(); }`. ROM printf is ISR-safe, followed by `esp_rom_uart_tx_wait_idle(0)` so the line is flushed before the reboot. No `__FILE__/__LINE__`, because they cost about 3 KB across the LVGL assert sites; WP2 can add a trace if it wants.
- CI: `.github/workflows/build.yml` fails the build job when `firmware.bin` > 1,550,000 B (today 1,498,208 B).
- F-04 (ARGB8888 off, -26 KB): tried, then REVERTED (orchestrator ruling). The default theme renders the dropdown list's rounded top/bottom strips (clip_corner + radius) into ARGB8888 layers that LVGL hard-codes (lv_theme_default.c:1004/310, lv_refr.c:207/228); with the format off they silently draw nothing, so the popup loses its border and corners. `LV_DRAW_SW_SUPPORT_RGB565A8` only covers opa_layered layers, not these. The src/ code search was not enough: theme code paths use layers too.

## Not done
- F-05 (built-in Montserrat 14 / `LV_FONT_DEFAULT`): left out. `font_14.c`, `font_18.c`, `font_24.c` and `font_clock_48.c` reference `lv_font_montserrat_14` as `.fallback`, and `src/ui/fonts/*.c` is owned by WP7. Disabling the built-in now breaks the link. After WP7 regenerates the fonts without the fallback, set `LV_FONT_MONTSERRAT_14 0` and `LV_FONT_DEFAULT &font_14` (declared via `LV_FONT_CUSTOM_DECLARE`) in lv_conf.h.
- `tools/check_text_fit.py` reports 5 overflows in 81 checks; no source or font was touched by this WP, so this is not caused by it.

## Hardware checks still needed
- `PERF_TEST` Settings scroll >= 40 fps (was 58-60 at -O2). If low, add the file to `HOT` in `tools/perf_flags.py`.
- All 13 screens, overlays, the dropdown popup (rounded corners and border) and touch calibration look identical (covers the 8 KB layer buffer).
- Forced assert (temporary `LV_MEM_SIZE 16K`) prints `[LV] assert` and reboots.
