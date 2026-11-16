#include "hamalert_parse.h"
#include "pota_parse.h"
#include "dx_parse.h"
#include "band_mode.h"
#include <string.h>

static bool parseJsonLine(const char* line, uint32_t nowUtc, Spot& dst) {
  JsonDocument doc;
  if (deserializeJson(doc, line) || !doc.is<JsonObject>()) return false;
  JsonObjectConst r = doc.as<JsonObjectConst>();
  const char* call = r["dx"] | r["call"] | r["callsign"] | "";
  double f = 0;
  if (!*call || !(jsonToDouble(r["frequency"], f) || jsonToDouble(r["freq"], f)) || f <= 0) return false;
  // Values above 1000 are kHz, otherwise MHz (same rule as the original).
  double khz = f > 1000.0 ? f : f * 1000.0;
  if (khz >= 1e7) return false;
  Spot s{};
  copyPrintable(s.call, sizeof(s.call), call);
  copyPrintable(s.spotter, sizeof(s.spotter), r["de"] | r["spotter"] | r["source"] | "");
  copyPrintable(s.comment, sizeof(s.comment), r["comment"] | r["info"] | r["message"] | "");
  copyPrintable(s.country, sizeof(s.country), r["country"] | r["entity"] | "");
  s.khz = (uint32_t)(khz + 0.5);
  s.band = bandFromKhz(s.khz);
  const char* mode = r["mode"] | "";
  s.mode = modeFrom(*mode ? mode : s.comment, s.khz);
  const char* ts = r["time"] | r["spotTime"] | r["timestamp"] | "";
  if (!isoUtcToUnix(ts, s.utc)) s.utc = nowUtc;
  s.distKm = -1;
  dst = s;
  return true;
}

bool hamalertParseLine(const char* line, uint32_t nowUtc, Spot& out) {
  if (!line) return false;
  while (*line == ' ' || *line == '\t') line++;
  if (*line == '{') return parseJsonLine(line, nowUtc, out);
  return parseDxLine(line, nowUtc, out);
}
