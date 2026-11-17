#pragma once
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "lvgl.h"
#include "types.h"

// Shared by the Weather and Forecast screens (UI task only).
lv_obj_t* wxIconCreate(lv_obj_t* parent, int x, int y);  // 50x50 slot: image, or "?" circle when the id is unknown
void wxIconSet(lv_obj_t* slot, int weatherId, bool night);
lv_color_t wxLevelColor(int level);  // 0..4 (wx_map.h levels): green, yellow, orange, red, dark red
lv_color_t wxWindColor(float ms);
void wxLocalClock(char* out, size_t n, struct tm* tmOut = nullptr);  // "HH:MM" in the UTC/Local mode of the clock screen; "--:--" until the clock is valid
// Footer text (age from the current or the forecast fetch): "Weather for location: <city>  (N min)" or the no-key / no-data message.
void wxFooter(char* out, size_t n, const WeatherData& w, bool haveKey, bool forecast = false);
