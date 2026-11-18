#pragma once
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#include "i18n.h"
#include "prop_level.h"
#include "../../app/data_store.h"
#include "../theme.h"

// Good/Fair/Poor colour; grey otherwise.
inline lv_color_t propCondColor(const char* s) {
  switch (propCondOf(s)) {
    case PropCond::Good: return lv_color_hex(0x00FF00);
    case PropCond::Fair: return lv_color_hex(0xFFFF00);
    case PropCond::Poor: return lv_color_hex(0xFF0000);
    default: return COL_GREY;
  }
}

// Translated condition word for the XML value, or the raw text.
inline const char* propCondText(const char* s) {
  if (!strcasecmp(s, "Good")) return tr(Tr::Good);
  if (!strcasecmp(s, "Fair")) return tr(Tr::Fair);
  if (!strcasecmp(s, "Poor")) return tr(Tr::Poor);
  return s;
}

// "updated N min ago" from a Unix fetch time; empty if never fetched or the clock is not valid.
inline void propAgeText(uint32_t fetchedAt, char* out, size_t cap) {
  time_t now = time(nullptr);
  if (!fetchedAt || now < (time_t)fetchedAt) { out[0] = 0; return; }
  snprintf(out, cap, tr(Tr::UpdatedMinAgo), (int)((now - (time_t)fetchedAt) / 60));
}

// Version + age tracking shared by the Propagation and HF bands screens (UI task only).
struct PropFeed {
  uint32_t ver = ~0u, ageAt = 0;
  PropagationData d{};

  // Re-reads the store when it changed and calls onChange(d); then shows the "updated" stamp / error flag and,
  // at most every 30 s (or on change), the age text.
  template <class F> void refresh(uint32_t now, lv_obj_t* upd, lv_obj_t* age, lv_obj_t* err, F onChange) {
    bool changed = g_store.prop.version() != ver;
    if (changed) {
      ver = g_store.prop.version();
      g_store.prop.read([&](const PropagationData& s) { d = s; });
      onChange(d);
      lv_label_set_text(upd, d.valid ? d.updated : "--");
      lv_obj_set_flag(err, LV_OBJ_FLAG_HIDDEN, d.valid);
    }
    if (changed || now - ageAt >= 30000) {
      ageAt = now;
      char a[40];
      propAgeText(d.fetchedAt, a, sizeof a);
      lv_label_set_text(age, a);
    }
  }
};
