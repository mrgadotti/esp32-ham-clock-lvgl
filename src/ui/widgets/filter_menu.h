#pragma once
#include <stdint.h>
#include "lvgl.h"

// Full-screen modal on lv_layer_top: mode tiles (ALL/CW/SSB/DIGI) and band tiles (ALL + `bands`, Band values).
// OK writes the edited masks to *bandMask / *modeMask and calls onSave; CANCEL discards.
// Optional extra button (top right, only when extraLabel is set): runs onExtra and closes the menu without saving. Masks as in passesFilter().
void filterMenuOpen(lv_obj_t* parent, const uint8_t* bands, uint8_t nBands, uint16_t* bandMask, uint8_t* modeMask,
                    void (*onSave)(), const char* extraLabel = nullptr, void (*onExtra)() = nullptr);
bool filterMenuIsOpen();
void filterMenuClose();
