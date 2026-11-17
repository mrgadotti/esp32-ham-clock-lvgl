/**
 * @file lv_conf.h
 * LVGL 9.6 configuration for the ESP32 ham clock (240x320 RGB565, no PSRAM).
 * Only options that differ from lvgl/include/lvgl/config/lv_conf_internal.h are listed here;
 * everything else takes the library default.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* For LV_ASSERT_HANDLER below (C and C++ safe). */
#include <esp_rom_sys.h>
#include <esp_system.h>

/* LV_COLOR_FORMAT_DEFAULT is RGB565 by default (was LV_COLOR_DEPTH 16, deprecated in 9.6). */

/* 40 KB pool: the ESP32 has no PSRAM and dram0_0_seg overflows once the Wi-Fi stack is linked in. */
#define LV_MEM_SIZE (40 * 1024U)

#define LV_DEF_REFR_PERIOD 16 /*[ms]*/
#define LV_DPI_DEF 130        /*[px/inch]*/

/*--- Software renderer: RGB565 (display, icons), RGB565A8 (opa_layered layers) and ARGB8888. Keep ARGB8888: LVGL hard-codes it
 *    for corner-clip layers (the dropdown list's rounded strips); with it off they silently draw nothing. ---*/
#define LV_DRAW_BUF_STRIDE_ALIGN 1
#define LV_DRAW_BUF_ALIGN 4
/* Layered draws (opacity, transform, some shadows) borrow this from LV_MEM; keep it small. */
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE (8 * 1024) /*[bytes]*/
#define LV_DRAW_SW_SUPPORT_RGB565 1
#define LV_DRAW_SW_SUPPORT_RGB565A8 1
#define LV_DRAW_SW_SUPPORT_RGB888 0
#define LV_DRAW_SW_SUPPORT_XRGB8888 0
#define LV_DRAW_SW_SUPPORT_ARGB8888 1
#define LV_DRAW_SW_SUPPORT_L8 0
#define LV_DRAW_SW_SUPPORT_AL88 0
#define LV_DRAW_SW_SUPPORT_A8 0
#define LV_DRAW_SW_SUPPORT_I1 0
#define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
#define LV_DRAW_SW_CIRCLE_CACHE_SIZE 4
#define LV_USE_DRAW_SW_COMPLEX_GRADIENTS 0

/*--- Asserts: a failed allocation or NULL logs over the console and reboots instead of hanging the UI task.
 *    esp_rom_printf is ROM code and ISR-safe; esp_rom_uart_tx_wait_idle flushes the line; esp_restart() does not return.
 *    No __FILE__/__LINE__: they cost about 3 KB of flash across the LVGL assert sites. ---*/
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_ASSERT_HANDLER { esp_rom_printf("[LV] assert\n"); esp_rom_uart_tx_wait_idle(0); esp_restart(); }

#define LV_GRADIENT_MAX_STOPS 2
#define LV_OBJ_STYLE_CACHE 0 /* default 1 costs RAM per object */
#define LV_BUILD_EXAMPLES 0
#define LV_USE_OBJ_ID 0
#define LV_USE_OBJ_ID_BUILTIN 1
#define LV_USE_OBJ_PROPERTY 0
#define LV_USE_OBJ_PROPERTY_NAME 0

#define LV_USE_FLOAT 1 /* lv_label_set_text_fmt %f needs it; 0 misaligns varargs */
#define LV_USE_MATRIX 0

/*--- Fonts: the app ships its own (src/ui/fonts); only Montserrat 14 is the built-in fallback/default ---*/
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*--- Widgets in use: obj, label, button, image, checkbox, dropdown, slider (needs bar), switch, table ---*/
#define LV_USE_ANIMIMG 0
#define LV_USE_ARC 0
#define LV_USE_ARCLABEL 0
#define LV_USE_BAR 1
#define LV_USE_BUTTON 1
#define LV_USE_BUTTONMATRIX 0
#define LV_USE_CALENDAR 0
#define LV_USE_CANVAS 0
#define LV_USE_CHART 0
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMAGE 1
#define LV_USE_IMAGEBUTTON 0
#define LV_USE_KEYBOARD 0
#define LV_USE_LABEL 1
#define LV_USE_LED 0
#define LV_USE_LINE 0
#define LV_USE_LIST 0
#define LV_USE_MENU 0
#define LV_USE_MSGBOX 0
#define LV_USE_ROLLER 0
#define LV_USE_SCALE 0
#define LV_USE_SLIDER 1
#define LV_USE_SPAN 0
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 0
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 0
#define LV_USE_TABLE 1
#define LV_USE_TABVIEW 0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0

#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 0
#define LV_THEME_DEFAULT_GROW 1
#define LV_THEME_DEFAULT_TRANSITION_TIME 80
#define LV_USE_THEME_SIMPLE 1
#define LV_USE_THEME_MONO 0

#define LV_USE_OBSERVER 0

#endif /*LV_CONF_H*/
