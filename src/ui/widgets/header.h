#pragma once
#include "lvgl.h"
#include "i18n.h"

constexpr int HEADER_H = 26;

// 26 px accent bar: title (left), free text such as "HH:MMZ" (right).
lv_obj_t* headerCreate(lv_obj_t* parent, Tr title);
void headerSetClock(lv_obj_t* header, const char* text);
// Shows the current UTC "HH:MM Z" ("--:--" until time is valid); touches the label only on change.
void headerTickUtc(lv_obj_t* header);
