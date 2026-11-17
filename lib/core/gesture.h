#pragma once
#include <stdint.h>
// Copy of ScreenId values (src/ui/screen.h) so lib/core has no LVGL dependency; static_asserts in registry.cpp keep them in sync.
namespace scr {
constexpr uint8_t HamClock = 1, DxCluster = 2, SunSpots = 3, BandInfo = 4, Weather = 5, AprsList = 6,
                  Pota = 7, HamAlert = 8, AprsRadar = 9,
                  Matrix = 10, Game = 11,  // reserved (removed): never created, skipped by nextScreen
                  Forecast = 12,
                  Location = 13, Settings = 14;
}
enum class Gesture : uint8_t { None, SwipeLeft, SwipeRight };
Gesture classifyGesture(int dx, int dy);
// order: 12 screen ids (0 = unused); enabledMask bit = screen id. Settings is always last.
// Matrix and Game (and any id not in the carousel) are never reachable; from such a cur, returns the first entry.
uint8_t nextScreen(const uint8_t order[12], uint16_t enabledMask, uint8_t cur, int dir);
// Boot screen from the stored id: a carousel screen that is still enabled, else the Clock (Settings, Matrix/Game, 0, garbage).
uint8_t resolveStartScreen(uint8_t stored, uint16_t enabledMask);
// Auto-switch step: next enabled carousel screen after cur, never Settings; wraps to the first. Returns cur if none enabled.
uint8_t nextAutoSwitchScreen(const uint8_t order[12], uint16_t enabledMask, uint8_t cur);
