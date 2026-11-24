#pragma once
#include <stddef.h>
#include "types.h"
// Parses a TNC2 line carrying an APRS message addressed to myCall (login call, "-SSID" appended when SSID > 0).
// Fills from/text/msgNo; utc and hasLatLon stay 0, sym = two spaces, distKm = -1 (the ingest step fills them).
// Returns false for other addressees, bulletins, ack/rej frames, empty text and non-message frames.
bool aprsParseMessage(const char* tnc2, const char* myCall, AprsMessage& out);
// True when from, msgNo and text are all equal (a sender retry).
bool aprsMsgSame(const AprsMessage& a, const AprsMessage& b);
// Formats the ack info field ":TO       :ackNNN" (addressee padded to 9, no msgNo of its own).
// False, out untouched, when to is empty or > 9 chars, msgNo is empty, or cap is too small.
bool aprsFormatAckInfo(const char* to, const char* msgNo, char* out, size_t cap);
