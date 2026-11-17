#pragma once
#include <stddef.h>
#include <stdint.h>

#ifndef FW_VERSION
#define FW_VERSION "2.0.0-dev"
#endif

// Callable from any task: formats one line into the boot-log ring (never log secrets) and mirrors it to Serial.
void bootLog(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
uint32_t bootLogVersion();
// Copies up to max of the newest lines (oldest first, 40 bytes each); returns how many.
size_t bootLogSnapshot(char (*out)[40], size_t max);
