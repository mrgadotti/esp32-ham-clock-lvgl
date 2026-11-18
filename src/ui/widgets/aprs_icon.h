#pragma once
#include "lvgl.h"
#include "aprs_symbol.h"

// Bitmap for `icon` at the nearest generated size >= px (16, 24, 32 or 48); nullptr for AprsIcon::None.
const lv_image_dsc_t* aprsIconImage(AprsIcon icon, int px);
