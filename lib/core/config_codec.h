#pragma once
#include <stddef.h>

#include "config.h"

struct ApplyResult {
  bool ok;
  bool needsRestart;
  char error[64];
  bool hasLastScr;  // request carried a valid "last_scr" (NVS key outside Config): the caller stores lastScr
  uint8_t lastScr;
  bool tcalSet;     // request carried a valid "tcal" (now in Config): the caller applies it to the touch driver
};

void configDefaults(Config&);
// Clamps every numeric field to its table range and terminates strings.
void configClamp(Config&);
// Secrets are emitted as "<key>_set": bool. False if buf is too small.
bool configToJson(const Config&, char* buf, size_t len);
// Same keys as POST /api/save accepts, with every secret in PLAIN TEXT (no "_set" flags). For the backup download only: never log the output.
// lastScr (the NVS "last_scr" byte) is emitted normalised through resolveStartScreen so the file always re-imports.
bool configToBackupJson(const Config&, char* buf, size_t len, uint8_t lastScr = 0);
// Partial update applied in place, all-or-nothing: on error the Config is untouched (no temporary Config copy). Empty secret strings keep the old value.
ApplyResult configApplyJson(Config&, const char* json);
// The same in two steps, so a caller can parse before taking a lock and keep only apply() inside it.
// (Opaque on purpose: this header must not pull ArduinoJson, whose <cmath> breaks files that declare a `log`.)
class ConfigPatch {
 public:
  explicit ConfigPatch(const char* json);  // parses only
  ~ConfigPatch();
  ConfigPatch(const ConfigPatch&) = delete;
  ConfigPatch& operator=(const ConfigPatch&) = delete;
  ApplyResult parsed() const;                 // ok=false: bad JSON
  ApplyResult apply(Config&) const;           // validate + commit, all-or-nothing, no parsing
 private:
  struct Impl;
  Impl* p_;
};

// Widest field (cluster_filters, aprs_alert): the in-place apply parses fields into a scratch of this size.
constexpr size_t kMaxFieldSize = 128;
