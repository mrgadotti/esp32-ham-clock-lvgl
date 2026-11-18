#pragma once
#include "lvgl.h"

#define COL_BG     lv_color_hex(0x000000)
#define COL_TEXT   lv_color_hex(0xFFFFFF)
#define COL_ACCENT lv_color_hex(0xFF8C00)
#define COL_FREQ   lv_color_hex(0xFFFF00)
#define COL_MODE   lv_color_hex(0x00FF00)
#define COL_ERROR  lv_color_hex(0xFF0000)
#define COL_GREY   lv_color_hex(0xC0C0C0)
#define COL_DIM    lv_color_hex(0x808080)
#define COL_CYAN   lv_color_hex(0x00FFFF)

constexpr int SCREEN_W = 320, SCREEN_H = 240;

// Installs the dark theme (black background, white text, font_14) on the display.
void themeApply(lv_display_t* disp);
