#include "boot_log.h"

#include <Arduino.h>
#include <stdarg.h>

#include "boot_ring.h"

static BootLogRing ring;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;  // ~400 B copies: cheaper than a mutex, usable before any init

void bootLog(const char* fmt, ...) {
  char buf[kBootLogLen + 24];  // longer text is cut by the ring
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  va_end(ap);
  Serial.printf("[BOOT] %s\n", buf);
  portENTER_CRITICAL(&mux);
  ring.push(buf);
  portEXIT_CRITICAL(&mux);
}

uint32_t bootLogVersion() { return ring.ver; }

size_t bootLogSnapshot(char (*out)[40], size_t max) {
  static_assert(kBootLogLen == 40, "snapshot row width");
  portENTER_CRITICAL(&mux);
  size_t n = ring.count < max ? ring.count : max;
  for (size_t i = 0; i < n; i++) memcpy(out[i], ring.at(ring.count - n + i), kBootLogLen);
  portEXIT_CRITICAL(&mux);
  return n;
}
