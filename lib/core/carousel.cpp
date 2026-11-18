#include "carousel.h"
#include <stdlib.h>

Gesture classifyGesture(int dx, int dy) {
  if (abs(dx) <= 40 || abs(dx) <= 2 * abs(dy)) return Gesture::None;
  return dx < 0 ? Gesture::SwipeLeft : Gesture::SwipeRight;
}

bool isCarouselScreen(uint8_t id) { return id >= 1 && id <= 12; }

// Enabled carousel screens in slot order, without Settings; the Clock is always included (it hosts the gear that opens Settings).
static int carouselList(const uint8_t order[12], uint16_t enabledMask, uint8_t list[12]) {
  int n = 0;
  for (int i = 0; i < 12; ++i) {
    uint8_t id = order[i];
    if (isCarouselScreen(id) && ((enabledMask | (1u << scr::HamClock)) & (1u << id))) list[n++] = id;
  }
  return n;
}

uint8_t nextScreen(const uint8_t order[12], uint16_t enabledMask, uint8_t cur, int dir) {
  uint8_t list[12]; int n = carouselList(order, enabledMask, list);
  if (n == 0) return scr::HamClock;
  int idx = -1;
  for (int i = 0; i < n; ++i) if (list[i] == cur) idx = i;
  if (idx < 0) return list[0];
  return list[(idx + (dir < 0 ? n - 1 : 1)) % n];
}

uint8_t nextAutoSwitchScreen(const uint8_t order[12], uint16_t enabledMask, uint8_t cur) {
  uint8_t list[12]; int n = carouselList(order, enabledMask, list);
  if (n == 0) return cur;
  int idx = -1;
  for (int i = 0; i < n; ++i) if (list[i] == cur) idx = i;
  return list[(idx + 1) % n];
}

uint8_t resolveStartScreen(uint8_t stored, uint16_t enabledMask) {
  bool ok = isCarouselScreen(stored) && (enabledMask & (1u << stored));
  return ok ? stored : scr::HamClock;
}
