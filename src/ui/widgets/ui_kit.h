#pragma once
#include <stdint.h>
#include "lvgl.h"

// Small helpers for LVGL code that was copy-pasted across the screens (label, rounded tile, header hamburger, modal).
// The UTC clock lives in header.h (headerTickUtc).

// Label with font, colour and text, placed at (x, y) of the parent (callers that align it afterwards can omit x/y).
// w > 0 fixes the width; align is only set when not AUTO.
lv_obj_t* uiLabel(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, const char* text, int x = 0, int y = 0,
                  int w = 0, lv_text_align_t align = LV_TEXT_ALIGN_AUTO);

// Clickable rounded tile with a centred font_14 label; `id` is passed to cb as user data (CLICKED).
lv_obj_t* uiTile(lv_obj_t* parent, const char* text, int x, int y, int w, int h, lv_event_cb_t cb, intptr_t id);
// Selected (accent) / unselected (grey) look of a uiTile.
void uiTilePaint(lv_obj_t* tile, bool on);

// Adds a 50x26 clickable three-bar button (bars are not clickable) at the left of a headerCreate() bar and moves the title right of it.
lv_obj_t* uiHamburger(lv_obj_t* header, lv_event_cb_t cb);

// Opaque full-screen panel on lv_layer_top that swallows touches (menus). Delete it with lv_obj_delete_async.
lv_obj_t* uiModalPanel();

// Transparent vertically scrollable box at (x, y) with a thin scrollbar, for menu rows that do not fit the screen.
lv_obj_t* uiScrollArea(lv_obj_t* parent, int x, int y, int w, int h);
