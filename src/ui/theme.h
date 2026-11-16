#pragma once
#include "lvgl.h"

#define COL_BG     lv_color_hex(0x000000)
#define COL_TEXT   lv_color_hex(0xFFFFFF)
#define COL_ACCENT lv_color_hex(0xFF8C00)
#define COL_FREQ   lv_color_hex(0xFFFF00)
#define COL_MODE   lv_color_hex(0x00FF00)
#define COL_ERROR  lv_color_hex(0xFF0000)

// Installs the dark theme (black background, white text, font_14) on the display.
void themeApply(lv_display_t* disp);
