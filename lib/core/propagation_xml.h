#pragma once
#include "types.h"
// Parses the hamqsl solar XML. Returns false (out untouched) if solarflux/kindex are missing.
// Does not set fetchedAt; the caller does.
bool parsePropagationXml(const char* xml, PropagationData& out);
