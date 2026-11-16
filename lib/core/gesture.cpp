#include "gesture.h"
#include <stdlib.h>

Gesture classifyGesture(int dx, int dy) {
  if (abs(dx) < 50 || abs(dy) > abs(dx) / 2) return Gesture::None;
  return dx < 0 ? Gesture::SwipeLeft : Gesture::SwipeRight;
}

uint8_t nextScreen(const uint8_t order[12], uint16_t enabledMask, uint8_t cur, int dir) {
  uint8_t list[13]; int n = 0;
  for (int i = 0; i < 12; ++i) {
    uint8_t id = order[i];
    if (id && id != scr::Settings && id != scr::Game && id < 16 && (enabledMask & (1u << id))) list[n++] = id;
  }
  list[n++] = scr::Settings;
  int idx = -1;
  for (int i = 0; i < n; ++i) if (list[i] == cur) idx = i;
  if (idx < 0) return list[0];
  return list[(idx + (dir < 0 ? n - 1 : 1)) % n];
}
