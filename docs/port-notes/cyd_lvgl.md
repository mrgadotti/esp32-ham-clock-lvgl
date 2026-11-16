# CYD 2432S028R + LVGL research

## Board (cyd-dev MCP)
- ESP32-WROOM-32, 4MB flash, no PSRAM, ILI9341 240x320 (landscape via rotation), XPT2046, PIO board esp32dev.
- TFT (HSPI): MISO12 MOSI13 SCLK14 CS15 DC2 RST -1, BL GPIO21 active HIGH (also on P3; plain GPIO/ledc PWM).
- Touch (own VSPI bus, NOT on TFT bus; no TOUCH_CS in TFT_eSPI): CLK25 MOSI(DIN)32 MISO(OUT)39 CS33 IRQ36. Raw ~200..3700 / 240..3800.
- RGB LED common-anode active LOW: R4 G16 B17. LDR GPIO34 (ADC1, safe with WiFi). SD SPI: SCK18 MISO19 MOSI23 CS5 (third bus; use XPT2046_Bitbang_Slim if all three used). Speaker GPIO26. BOOT GPIO0.
- Inversion guidance (verbatim): "Colour inversion varies between board revisions. The book's User_Setup.h enables TFT_INVERSION_ON with ILI9341_2_DRIVER. If your display shows a negative image, drop TFT_INVERSION_ON (or switch to plain ILI9341_DRIVER)." Generator emits -DILI9341_2_DRIVER=1 -DTFT_INVERSION_ON=1, no TFT_RGB_ORDER. Variant v3/CYD2USB = ST7789 + TFT_BGR + INVERSION_OFF (slug cyd2usb-2432s028rv3).
- Memory gotcha: LV_MEM_SIZE 64K + 1/10 draw buf + WiFi => dram0_0_seg overflow; fix 40K + 1/20-screen buf (what generator emits). Static DRAM problem, partition size irrelevant.

## Libraries
- Generator pins: lvgl/lvgl@^9.2.0 (resolved 9.6.0), bodmer/TFT_eSPI@^2.5.43 (2.5.43), XPT2046_Touchscreen from PaulStoffregen git (1.4.0+sha.f956c5d). (Book vendors lvgl 8.3.6 but sketches target 9.)
- Display path: LVGL built-in lv_tft_espi_create() (LV_USE_TFT_ESPI 1) over TFT_eSPI. LovyanGFX only appears in other-board examples.
- Platform resolved: espressif32 8.0.0 (arduino-esp32 3.3.x), unpinned in generated ini - pin it.

## Example patterns
- Touch: lv_indev POINTER, read cb: `if(ts.tirqTouched()&&ts.touched()){p=ts.getPoint(); x=map(p.x,200,3700,0,W); y=map(p.y,240,3800,0,H);}`; ts.setRotation(2); display LV_DISPLAY_ROTATION_270 in book examples (generator uses ROTATION_0 portrait 240x320). No example does on-screen calibration: roll own (4-corner/3-point tap, store min/max or affine coeffs in Preferences, load at boot).
- Slider/brightness: lv_slider_create + lv_obj_add_event_cb(..., LV_EVENT_VALUE_CHANGED); read lv_slider_get_value; drive GPIO21 with ledcAttach/ledcWrite (no example does BL PWM).
- List: lv_list_create, lv_list_add_text, lv_list_add_button(list, LV_SYMBOL_x, "txt"), CLICKED cb, lv_list_get_button_text. lv_list scrolls by drag natively (flex column). No lv_table/canvas/zoom examples in index; use lv_table / lv_canvas / lv_image_set_scale (LVGL 9) from LVGL docs.
- Loop: lv_tick_set_cb(millis) preferred; lv_timer_handler()+delay(5). Book uses lv_tick_inc(N)+delay(N).
- No Wi-Fi+HTTP+LVGL memory example; no custom font example. Fonts: lv_font_conv (npm) --bpp 4 --size N --font X.ttf --range 0x20-0x7F,0xA0-0xFF --format lvgl -o f.c, then LV_FONT_DECLARE; Latin-1 range covers Portuguese accents. Built-in Montserrat is ASCII-only+symbols (use as fallback via font.fallback). lv_conf has LV_USE_TINY_TTF/FREETYPE=0.

## Generated project files
platformio.ini, src/main.cpp, include/lv_conf.h, .gitignore, README.md
platformio.ini: espressif32, esp32dev, arduino, board_build.partitions=huge_app.csv, upload 921600, flags: USER_SETUP_LOADED, ILI9341_2_DRIVER, TFT 240x320, pins above, TFT_BL=21, TFT_BACKLIGHT_ON=HIGH, TFT_INVERSION_ON=1, fonts, SMOOTH_FONT, SPI_FREQUENCY=55000000, SPI_READ 20000000, USE_HSPI_PORT, LV_CONF_INCLUDE_SIMPLE, -I include.
lv_conf.h: LV_COLOR_DEPTH 16; LV_COLOR_16_SWAP absent (LVGL9; swap handled by lv_tft_espi driver); LV_USE_STDLIB_MALLOC BUILTIN, LV_MEM_SIZE 40*1024; LV_USE_LOG 1; Montserrat 8..48 ALL enabled; default font 14; LV_USE_TFT_ESPI 1; draw buf = W*H/20*2 = 7680 B (static uint32_t array).

## Builds
- Baseline (tft,touch,lvgl,wifi,ldr,rgb; huge_app 3MB): RAM 95896 B (29.3% of 327680); Flash 1760376 B (56.0% of 3145728).
- + WiFi/HTTPClient/WiFiClientSecure/WebServer/Preferences/LittleFS/ArduinoJson 7.4.3 dummy use: RAM 100040 B (30.5%); Flash 2040008 B (64.9%).
- Same, trimmed fonts (keep 14,20,28,...) + LV_USE_LOG 0: RAM 99896; Flash 1999148 (only -41KB).
- Partition fit: 0x160000 = 1441792 B -> does NOT fit (2.04MB). min_spiffs OTA slot 0x1E0000 = 1966080 -> does NOT fit either (even trimmed 1999148). Only single-app layouts (huge_app 3MB / default 1.25MB no) fit; OTA on 4MB needs ~1.5-1.9MB image -> would need dropping TLS/fonts etc. Static RAM is fine (~100KB); real constraint is heap at runtime (TLS handshake ~40-50KB contiguous) and flash.

## Native tests
`platform = native`, test_framework = unity, test/test_smoke/test_main.cpp with own main(): `pio test -e native` PASSED. gcc 15.2.0 present; PIO auto-installed native@1.2.1 and Unity 2.6.1. No setup needed. Project: scratchpad/native_t.
