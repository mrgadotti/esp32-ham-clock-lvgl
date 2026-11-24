#pragma once
#include <stdint.h>
void aprsBeaconTick(uint32_t nowMs);  // net task: periodic position beacon over APRS-IS or UART2 KISS
void aprsBeaconRequest();  // any task: beacon at the next tick if the channel is ready (resets the interval timer)
void aprsAckRequest(const char* to, const char* msgNo);  // any task: queue an APRS ack (max 4, oldest dropped); sent by the next tick
