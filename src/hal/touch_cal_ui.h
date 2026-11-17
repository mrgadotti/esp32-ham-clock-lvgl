#pragma once
#include <Arduino.h>

bool touchCalLoadSaved();  // applies the NVS "tcal" blob if present and valid; false otherwise
// Builds the calibration screen and returns immediately; an lv_timer drives the 3-point calibration
// (plus verify step), saves NVS "tcal", restores the previous screen and calls onDone(saved).
// Must be called from the UI task (the LVGL thread); lv_timer_handler() must keep running.
void touchCalRun(void (*onDone)(bool saved));
bool touchCalActive();  // true while the calibration screen is up (it is modal)
