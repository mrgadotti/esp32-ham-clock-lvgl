#pragma once
// Parses one TNC2 line, stores the station (moved to front by call), raises alert + LED if the rules match.
// source: 'I' APRS-IS, 'T' TNC2 UART, 'K' KISS. Callable from any task.
void aprsIngestLine(const char* tnc2, char source);
void aprsIngestInit();  // creates the mutex; call from setup() before the tasks start
