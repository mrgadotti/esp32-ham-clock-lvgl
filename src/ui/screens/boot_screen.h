#pragma once
#include "lvgl.h"

// Not a carousel screen: owned by UiManager while booting.
lv_obj_t* bootScreenCreate();         // returns the lv screen object (not loaded)
void bootScreenUpdate(bool wifiUp);   // refreshes the boot log lines
