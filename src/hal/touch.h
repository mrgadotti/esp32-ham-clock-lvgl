#pragma once
#include <Arduino.h>
#include <lvgl.h>

#include "gesture.h"
#include "touch_cal.h"

void touchInit();
bool touchReadRaw(int16_t& rx, int16_t& ry);  // false when not pressed; 4-sample average
lv_indev_t* touchCreateIndev();
void touchSetCal(const TouchCal&);
bool touchHeldAtBoot(uint32_t ms);  // true if touched continuously for ms (call after touchInit)
void touchSetMirror(bool mirror180);  // x=319-x, y=239-y after calibration
void touchSetGestureSink(void (*sink)(Gesture));
