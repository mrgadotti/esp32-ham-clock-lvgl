# Scroll performance report

Board: CYD ILI9341 320x240, LVGL 9.6, no PSRAM, WiFi connected (real network traffic during the runs).
Method: `PLATFORMIO_BUILD_FLAGS=-DPERF_TEST pio run -e cyd -t upload`. The benchmark waits for the boot screen, scrolls the
Settings list 10 s (6 px every 16 ms), jumps to the DX screen, injects 100 fake rows into the spot table (PERF_TEST only) and
scrolls it 10 s. Numbers are the mean of the middle seconds. `-DPERF_PROF` adds a per-function self-time dump (`PROF` lines).

## Key finding

The firmware was not slow because of the renderer alone. The code runs from the flash cache, and with the stock 40 MHz
flash every cache miss in the LVGL draw path was expensive: the same code ran about 6x slower than the earlier reports
(Settings 9 fps, not the 50 fps in the old perf report; that old figure was probably measured before the boot screen
swallowed the `uiRequest`, i.e. on a screen that was not scrolling). After that the limits are: per-chunk traversal
overhead (many small draw chunks) and, for Settings, the SPI link (68k px per frame at an effective 40 MHz = about 27 ms).

## Experiments

fps = scroll frames per second. Settings / DX table (100 rows).

| # | Change | Settings | DX 100 rows | Result |
|---|--------|----------|-------------|--------|
| 0 | Baseline main (-Os + -O2 hybrid, 40 MHz flash, 1x 320x20 buffer, CPU swap flush) | 9 | 5.5 | |
| 1 | Flash 80 MHz QIO (`board_build.f_flash`, `flash_mode`) | 15 | 11 | KEPT (biggest single win) |
| 2 | Removing `-O2` hot-file hybrid (plain `-Os`) | 16.7 | 12 | KEPT: smaller code misses the cache less, -5 KB flash |
| 3 | RGB565_SWAPPED rendering alone (no CPU swap in flush) | 15.1 | 11 | no gain alone, needed for DMA |
| 4 | Two 320x10 buffers, DMA flush overlapped with render | 18.7 | 11 | |
| 5 | Two 320x20 buffers (25.6 KB heap) | 28.6 | 19 | KEPT (per-chunk overhead halves). Largest free block unchanged (34.8 KB idle, same as main); total free heap -12.8 KB |
| 6 | Two 320x30 buffers | 30.6 | 23 | REJECTED: idle largest block fell to 19 KB (TLS needs 2 x 16.6 KB) |
| 7 | `LV_OBJ_STYLE_CACHE 1` | 29.9 | 17.8 | REJECTED: no gain, less LV_MEM |
| 8 | Custom spot table instead of `lv_table` | 29 | 25 | KEPT. Also frees ~15 KB of LV_MEM on the DX screen (lvfree 16 KB -> 31.8 KB) and does no redraw when a refresh brings identical rows |
| 9 | SPI_FREQUENCY 80 MHz | 18 | 19 | REJECTED: slower here and out of ILI9341 spec; picture correctness cannot be judged without eyes |
| 10 | Net task off (WiFi/TLS not running) | 10 (vs 9) | 6.5 (vs 5.5) | not a factor, not kept |

Final: Settings about 29 fps (was 9), DX 100-row table about 25 fps (was 5.5). Flash `firmware.bin` 1,473,872 B (limit 1,550,000).
RAM static unchanged (119,132 B). Idle largest free heap block 34.8 KB (same as main); free heap about 103 KB (main: 122 KB)
because of the second draw buffer. lvfree on the DX screen 31.8 KB (main: 16 KB).

## Why the targets were not fully reached

- Settings (>= 40 fps asked): 68k px per frame over an effective 40 MHz SPI link is 27 ms of pure transfer. At 29 fps the
  frame is already close to that bound, so more render optimisation cannot help. Only fewer pixels per frame or a faster
  SPI would. 80 MHz was slower in the test (not understood) and is out of spec, so it was not enabled.
- DX table (>= 30 fps asked): 25 fps. Remaining cost is about 700 glyphs per frame at about 20 us each in LVGL's letter path
  plus draw-task creation, and the 25 ms SPI transfer. A bigger buffer would get to about 30 fps but breaks the heap budget (row 6).
- Ideas not done: hardware scroll (the panel scrolls along the 320 px axis, our scroll is along the 240 px axis in landscape),
  pausing label/table updates while a finger is down (the table refresh is now no-op when rows are unchanged, and the
  Settings info labels only change when their text changes, so the benefit would be small).

## Verification done

- `pio test -e native` 177 pass; `pio run -e cyd -e esp32-38pin` OK; `tools/check_text_fit.py` 0 overflow.
- cyd_test harness: all screens EN/PT screenshots, zero failures; POTA table shows correct colours, zebra and cropping;
  swipe navigation and the filter dialog tap still work. `tools/screen_test.py` now decodes big-endian pixels.
- Repeated PERF runs (6+ boots) without a crash or LVGL assert.

## Check by eye on the board (not verifiable from here)

1. Colours and inversion on every screen: the flush now sends the LVGL buffer unchanged (RGB565_SWAPPED + DMA). Wrong
   red/blue or odd byte colours would mean this commit has to be reverted. Toggle "Invert colors" in Settings too.
2. Flash at 80 MHz QIO: the bootloader is rewritten by `pio run -t upload`, an OTA update does not change it. If a
   particular board does not boot, revert the platformio.ini flash lines.
3. Table screens (DX, POTA, HamAlert, APRS): zebra stripes, column colours, header separator, text cropped at the column edge,
   scroll to the end and back, and a row-count change while scrolled (the position is clamped).
4. Drag feel in Settings and in tables; swipe between screens and dropdown/filter popups while scrolled.
5. Run a TLS refresh (weather/POTA) while the board sits on a table screen and watch the serial `[MEM]` line for `largest`
   (idle 34.8 KB) and for `SSL alloc` errors.
