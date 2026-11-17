#pragma once
#include <stddef.h>

#include "config.h"

struct ApplyResult {
  bool ok;
  bool needsRestart;
  char error[64];
};

void configDefaults(Config&);
// Clamps every numeric field to its table range and terminates strings.
void configClamp(Config&);
// Secrets are emitted as "<key>_set": bool. False if buf is too small.
bool configToJson(const Config&, char* buf, size_t len);
// Partial update applied in place, all-or-nothing: on error the Config is untouched (no temporary Config copy). Empty secret strings keep the old value.
ApplyResult configApplyJson(Config&, const char* json);
