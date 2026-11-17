#pragma once
#include "lvgl.h"
#include "i18n.h"

constexpr int HEADER_H = 26;

// 26 px accent bar: title (left), free text such as "HH:MMZ" (right).
lv_obj_t* headerCreate(lv_obj_t* parent, Tr title);
void headerSetClock(lv_obj_t* header, const char* text);
// Offset in seconds that every screen adds to UTC for display: 0 for UTC, the configured timezone when the clock
// screen is toggled to local (cfg().screen1_time). The single source of truth for UTC/Local.
int32_t uiTimeOffsetSec();
// Shows the current time "HH:MM Z" (UTC) or "HH:MM L" (local) ("--:--" until time is valid); touches the label only on change.
void headerTickUtc(lv_obj_t* header);
