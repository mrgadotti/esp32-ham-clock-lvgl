#pragma once
#include <stdint.h>
uint32_t backoffMs(uint8_t failures);  // 5 s doubling, capped at 300 s
uint32_t listRetryMs(uint8_t failures);  // retry for an empty list after a failed fetch: 30 s doubling, capped at 120 s
