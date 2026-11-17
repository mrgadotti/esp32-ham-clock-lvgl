# Diagnosis: touch dead on a second CYD

## Hardware identification (read from the board)
- ESP32-D0WD-V3 rev v3.1, 40 MHz crystal, 4 MB flash (manufacturer 0x5e, device 0x4016), DIO boot, as expected for the `cyd` env.
- Same variant as the development board: ILI9341 (`ILI9341_2_DRIVER`) with XPT2046 on VSPI (CLK 25, MOSI 32, MISO 39, CS 33, IRQ 36). The boot log reports `Display 320x240 rot 1 inv 1` (imported tft_rotation/tft_invert applied); screen captures show correct colours and orientation.
- NVS works: the imported config (callsign, WiFi, language) survives repeated reboots. `Touch calibration needed` is logged because the backup has no `tcal` key.

## Touch hardware is alive
Temporary diagnostics (removed, not committed) issued raw XPT2046 reads every 500 ms, bypassing the IRQ gate:
`z1=2 z2=~4088 x=0 y=~1060`, IRQ pin idle high, MISO not stuck. A dead or miswired chip would read constant 0 or 4095 on all channels; here the pressure channels give the textbook idle values, so SPI wiring, pins and the chip are fine. Pressed values were not captured (needs a finger on the panel), so the `z < 400` threshold is not shown to be wrong.

## Root cause (software)
`uiTask` ran `touchCalRun()` (no `tcal` -> calibration needed) and then `uiInit()`, which does `lv_screen_load(bootObj)`; the boot hand-over later loads the clock screen. The calibration screen was covered immediately, while its timer kept running with `touchEnable(false)`:
- the UI indev was disabled, so no tap reached the clock: "touch does not work at all";
- `POST /api/reset_touch` -> `StartTouchCal` -> `touchCalRun` returned early (`if (S.timer) return;`, "already running"): "does nothing".

Evidence: a screen capture of the old firmware 15 s after boot shows the clock screen while the invisible calibration is still pending. It goes unnoticed on a board that already has a `tcal`; any board without one (fresh flash plus a backup lacking `tcal`) hits it. The web flasher image is not a factor (same code path).

## Fix
- `main.cpp`: start the calibration after `uiInit()`.
- `ui_manager.cpp`: `uiTick` does nothing while `touchCalActive()` (new, in `touch_cal_ui`), so the boot hand-over, auto-switch and overlays cannot cover the modal calibration (also protects a web-triggered recalibration).

After the fix, a capture 15 s after boot shows the calibration cross and it stays.

## What the user must do
Touch the three crosses, then the verify point; the calibration is saved to NVS. Afterwards "recalibrate" from the web page or settings works. Holding the screen at boot (2 s) forces it (unchanged `touchHeldAtBoot`, not exercised here).
If taps are ignored on the cross, the pressure threshold (`p.z < 400` in `touch.cpp`) is the next suspect.
