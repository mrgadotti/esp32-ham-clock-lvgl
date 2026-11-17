#pragma once
#include <ArduinoJson.h>
#include "types.h"
// All parsers return false and leave `out` untouched on garbage / API errors.
// Text is sanitized to printable ASCII, EXCEPT valid UTF-8 for U+00A0..U+00FF (Latin-1) which is kept
// because the fonts cover it (pt_br descriptions have accents).
bool weatherParseCurrent(JsonDocument& doc, WeatherData& out);  // /weather; does not set fetchedAt
// /forecast. nowUnix (UTC secs, 0 = clock unknown) and tzOffsetSec pick day slots and the 6 h alert window.
bool weatherParseForecast(JsonDocument& doc, WeatherData& out, long nowUnix = 0, long tzOffsetSec = 0);
bool weatherParseAir(JsonDocument& doc, WeatherData& out);      // /air_pollution
bool isNightFromIcon(const char* iconCode);                     // icon code ends with 'n'
