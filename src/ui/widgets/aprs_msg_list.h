#pragma once
#include "lvgl.h"

// Modal panel on lv_layer_top listing the stored APRS messages (newest first). Tapping a row re-opens the alert.
// Back arrow closes only the panel; X closes it and then calls onExit (may be null). No-op if already open.
void aprsMsgListOpen(void (*onExit)());
bool aprsMsgListIsOpen();
void aprsMsgListClose();  // safe when not open
