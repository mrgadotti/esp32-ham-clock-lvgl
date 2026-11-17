#pragma once
// Screen-test harness, compiled only with -DSCREEN_TEST (env cyd_test). See tools/screen_test.py.
#ifdef SCREEN_TEST
#include <lvgl.h>
#include <stdint.h>

void screenTestInit();                                    // after Serial.begin
void screenTestPoll();                                    // UI task, every loop pass: serial command parser
bool screenTestTouch(int16_t& x, int16_t& y);             // indev read cb: true while a synthetic press is active
bool screenTestCapturing();                               // display flush cb: stream rects when true
void screenTestFlush(const lv_area_t* a, const uint8_t* px, bool last);  // display flush cb
#endif
