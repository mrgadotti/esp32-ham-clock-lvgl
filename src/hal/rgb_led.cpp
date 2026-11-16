#include "rgb_led.h"

#include <Arduino.h>
#include <atomic>

#include "board.h"

#ifdef HAS_RGB_LED
namespace {
std::atomic<uint32_t> until{0}, half{0};
std::atomic<uint8_t> colorMask{0};
std::atomic<bool> pending{false};  // set by rgbLedBlink (any task), consumed by rgbLedTick
uint32_t lastToggle;
bool on, inited;

void drive(uint8_t mask) {  // active LOW
  digitalWrite(PIN_LED_R, !(mask & RGB_RED));
  digitalWrite(PIN_LED_G, !(mask & RGB_GREEN));
  digitalWrite(PIN_LED_B, !(mask & RGB_BLUE));
}
}  // namespace

void rgbLedBlink(uint8_t color, uint32_t durationMs, uint32_t blinkMs) {
  colorMask = color;
  half = blinkMs < 50 ? 50 : blinkMs;
  until = millis() + durationMs;
  pending = true;  // set last: the tick reads the other fields after seeing it
}

void rgbLedTick(uint32_t now) {
  if (!inited) {
    pinMode(PIN_LED_R, OUTPUT);
    pinMode(PIN_LED_G, OUTPUT);
    pinMode(PIN_LED_B, OUTPUT);
    drive(0);
    inited = true;
  }
  if (pending.exchange(false)) {
    on = true;
    lastToggle = now;
    drive(colorMask.load());
    return;
  }
  if (!colorMask) return;
  if ((int32_t)(now - until.load()) >= 0) {
    colorMask = 0;
    on = false;
    drive(0);
  } else if (now - lastToggle >= half.load()) {
    lastToggle = now;
    on = !on;
    drive(on ? colorMask.load() : 0);
  }
}
#else
void rgbLedBlink(uint8_t, uint32_t, uint32_t) {}
void rgbLedTick(uint32_t) {}
#endif
