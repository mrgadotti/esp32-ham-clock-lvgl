#pragma once
#include <stdint.h>
uint32_t backoffMs(uint8_t failures);  // 5 s doubling, capped at 300 s
