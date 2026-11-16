#pragma once
#include <stdint.h>

constexpr uint8_t RGB_RED = 1, RGB_GREEN = 2, RGB_BLUE = 4;  // bit mask
// Blinks `color` for durationMs with a blinkMs half-period, then turns off. No-op without HAS_RGB_LED.
void rgbLedBlink(uint8_t color, uint32_t durationMs, uint32_t blinkMs);
void rgbLedTick(uint32_t now);  // net or ui task loop
