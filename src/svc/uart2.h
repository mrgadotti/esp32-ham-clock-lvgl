#pragma once
#include <ArduinoJson.h>
#include <stddef.h>
#include <stdint.h>

void uart2Task(void*);  // FreeRTOS task body: GPS NMEA, APRS TNC text/KISS, BT bridge on UART2
bool gpsTimeValid();    // system clock was set from GPS recently: NTP is skipped meanwhile
// Writes an already KISS-encoded frame to the TNC. False unless UART2 is open in KISS mode.
bool uart2SendKiss(const uint8_t* frame, size_t len);

// Bridge access for the web task (the FSM lives on the uart task, guarded by a mutex).
bool uart2BridgeEnabled();
void uart2BridgeStatus(JsonDocument& d);
bool uart2BridgeScan();
bool uart2BridgePair(const char* name, const char* addr, const char* password);
