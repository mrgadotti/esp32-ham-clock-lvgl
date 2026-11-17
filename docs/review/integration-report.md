# Integration report (wave 3 merges)

Branch main, merged with `--no-ff` in order. cyd = `pio run -e cyd`. Native tests: 170 before, 177 after WP6b.

| Step | Flash (B) | RAM (B) | Tests | Conflicts / resolution |
|---|---|---|---|---|
| Baseline (waves 1-2) | 1,737,417 | 116,476 | 170 | - |
| WP6c web CSRF/WiFi/time | 1,738,061 | 116,556 | 170 | none (uart2.cpp gpsToUnix merged cleanly) |
| WP6b sockets/TLS gate/HamAlert | 1,742,957 | 118,812 | 177 | none; uart2.cpp has a single gpsToUnix call |
| WP4 other screens on ui_kit | 1,745,665 | 118,948 | 177 | `time_service.cpp`: WP4 deleted dead `localTimeOf`, WP6c had edited it; deletion won, unused `config_store.h` include removed. `qrz.*`, `mem_log.*`, `check_text_fit.py` auto-merged (qrzEnqueue stays `static`, no external caller; `memLogSetLvFree` is still used). check_text_fit: 0 overflow |
| WP1 build flags, -Os/-O2 hybrid, lv_conf 9.6 | 1,472,969 | 118,812 | 177 | none textually. Re-checked lv_conf: `LV_USE_FLOAT 1`, `LV_DEF_REFR_PERIOD 16`, `LV_MEM_SIZE 40 KB`, `LV_USE_LOG` default 0, widget/draw-format trims all preserved |
| Screen-test harness | 1,472,969 | 118,812 | 177 | none textually (hooks in display/touch/main are `#ifdef SCREEN_TEST`; flag only in env `cyd_test`). Compile fix: harness used `memLogLvFree()` removed by WP4, now `lv_mem_monitor`. `nm` on the cyd ELF: no screen_test symbols; cyd flash unchanged by the merge |
| Follow-up a: font_14 default, no built-in Montserrat | 1,459,301 | 118,812 | 177 | `nm` shows no montserrat symbol; the ELF links (font fallbacks are already NULL). font_14 holds U+F00C (tick) and U+F078 (dropdown arrow) |
| Follow-ups b, c | 1,459,285 | 118,700 | 177 | - |

Other envs at the final commit: esp32-38pin 1,458,773 B / 118,676 B; cyd_test 1,460,673 B / 118,788 B; cyd with `-DPERF_TEST` 1,460,061 B / 118,740 B.

Follow-ups
- a. `LV_FONT_MONTSERRAT_14 0`, `LV_FONT_CUSTOM_DECLARE LV_FONT_DECLARE(font_14)`, `LV_FONT_DEFAULT &font_14`: -13.7 KB.
- b. APRS beacon project comment is `github.com/mrgadotti/esp32-ham-clock-lvgl` (41 chars; the https:// form is 49 and exceeds the 43-char APRS comment limit); `static_assert` guards the length. README has no beacon mention.
- c. `pend*` globals and `g_save*` globals replaced by lambda captures (aprs_list, spot_list, uart2); the stale "plain function pointer" comments are gone.
- d. No other file includes `app/config_store.h` without using it (`events.cpp` defines `onConfigChanged`, declared there). `qrzOnNewSpot` is declared once, in `svc/qrz.h`.

Not verified: nothing was flashed or run on hardware.
