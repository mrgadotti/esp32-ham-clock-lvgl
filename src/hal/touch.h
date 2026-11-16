#pragma once
#include <Arduino.h>
#include <lvgl.h>

#include "gesture.h"
#include "touch_cal.h"

void touchInit();
// false when not pressed. One sample: XPT2046_Touchscreen already averages internally (it re-samples
// at most every 3 ms and filters), so an extra loop here would be a no-op.
bool touchReadRaw(int16_t& rx, int16_t& ry);
lv_indev_t* touchCreateIndev();
void touchSetCal(const TouchCal&);
bool touchGetMirror();
bool touchHeldAtBoot(uint32_t ms);  // true if touched continuously for ms (call after touchInit)
void touchSetMirror(bool mirror180);  // x=319-x, y=239-y after calibration
void touchEnable(bool on);  // on=false stops the indev delivering clicks/gestures (used during calibration)
// The sink runs inside the indev read callback (LVGL thread): it must only set a flag, not touch LVGL.
void touchSetGestureSink(void (*sink)(Gesture));
