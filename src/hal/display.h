#pragma once
#include <Arduino.h>

void displayInit(uint8_t rotation, bool invert);
void displaySetBacklight(uint8_t percent);  // clamped to 5..100
void displaySetInvert(bool invert);
void displaySetRotation(uint8_t rot);  // 1 or 3
void displayApplyNightDim(bool night);  // UI task only: backlight = cfg().tft_backlight * (100 - tft_ndim) / 100 when night
