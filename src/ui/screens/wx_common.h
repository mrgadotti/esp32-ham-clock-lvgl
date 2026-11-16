#pragma once
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "lvgl.h"
#include "types.h"

// Shared by the Weather and Forecast screens (UI task only).
lv_obj_t* wxIconCreate(lv_obj_t* parent, int x, int y);  // 50x50 slot: image, or "?" circle when the id is unknown
void wxIconSet(lv_obj_t* slot, int weatherId, bool night);
lv_color_t wxWindColor(float ms);
void wxLocalClock(char* out, size_t n, struct tm* tmOut = nullptr);  // "HH:MM" local; "--:--" until the clock is valid
// Footer text: "Weather for location: <city>  (N min)" or the no-key / no-data message.
void wxFooter(char* out, size_t n, const WeatherData& w, bool haveKey);
lv_obj_t* wxLabel(lv_obj_t* parent, const lv_font_t* f, lv_color_t col, int x, int y, int w = 0,
                  lv_text_align_t al = LV_TEXT_ALIGN_LEFT);
