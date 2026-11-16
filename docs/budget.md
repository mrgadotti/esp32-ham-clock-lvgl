# Build budget

Baseline (Task 1: LVGL 9.6.0 + TFT_eSPI 2.5.43, color-bar test screen, no WiFi code yet):

| env | RAM (static) | Flash |
|---|---|---|
| cyd | 76260 B (23.3%) | 563321 B (17.9% of 3 MB) |
| esp32-38pin | 76260 B (23.3%) | 563337 B (17.9% of 3 MB) |

Limits: app flash <= 2.6 MB, static RAM <= 150 KB. WiFi/TLS/WebServer are not linked yet,
so expect roughly +5 KB RAM and +480 KB flash once they are (see port-notes/cyd_lvgl.md).
