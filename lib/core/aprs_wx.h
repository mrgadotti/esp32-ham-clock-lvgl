#pragma once
#include "types.h"
// Decodes weather fields from an APRS comment ("220/004g005t077...", optionally led by '_').
// Returns wx.hasAny. Orig decodeAprsWxFromComment L11900.
bool aprsDecodeWx(const char* comment, AprsWx& wx);
