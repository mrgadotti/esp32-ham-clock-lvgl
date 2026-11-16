#pragma once
#include "types.h"
// Parses "SRC>DST,PATH:info" (uncompressed, compressed, Mic-E, object, item positions).
// Fills call/sym (table,code)/comment/lat/lon/freqMhz/isWx; utc and source are left for the caller.
// distKm = -1 and bearing = -1 (unknown; computed later by the ingest step).
// Frames without a decodable position return false (the original returned true with no position).
bool aprsParseFrame(const char* tnc2Line, AprsStation& out);
// First "NNN.NNN MHz" (or a bare decimal in 30..1000) found in text.
bool aprsExtractFreq(const char* text, float& mhz);
