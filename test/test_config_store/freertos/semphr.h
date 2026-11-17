#pragma once
// Native stand-in: a non-recursive mutex that counts a take of an already-held lock (would deadlock on target).
struct StubSem {
  bool held = false;
};
typedef StubSem* SemaphoreHandle_t;
inline int g_stubDeadlocks = 0;
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return new StubSem; }
inline void xSemaphoreTake(SemaphoreHandle_t s, unsigned) {
  if (s->held) g_stubDeadlocks++;
  s->held = true;
}
inline void xSemaphoreGive(SemaphoreHandle_t s) { s->held = false; }
