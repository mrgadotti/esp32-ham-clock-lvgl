#pragma once
#include <stdint.h>
// Send one TNC2 line ("CALL>APRS,TCPIP*:info"); false unless logged in with a real passcode.
bool aprsIsSendLine(const char* line);
bool aprsIsReadyToSend();
void aprsIsTick(uint32_t now);  // net task: connect/login/filter/read; off without callsign or when UART2 is an APRS TNC
