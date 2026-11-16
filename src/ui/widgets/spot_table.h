#pragma once
#include <stddef.h>
#include <stdint.h>
#include "lvgl.h"

// Deviation from the brief: the widget is generic (Spot, AprsStation, ...) so POTA/HamAlert/APRS can reuse it.
// Instead of `rowAt(i, Spot&, ctx)` it asks for one cell text at a time; the callback takes the store lock itself.
constexpr uint8_t SPOT_TABLE_MAX_COLS = 6;
struct SpotTableCols {
  uint8_t n;
  int16_t width[SPOT_TABLE_MAX_COLS];
  lv_color_t color[SPOT_TABLE_MAX_COLS];
};
// Writes the text of (row, col) to out; false = leave the cell empty.
typedef bool (*CellFn)(size_t row, uint8_t col, char* out, size_t cap, void* ctx);

lv_obj_t* spotTableCreate(lv_obj_t* parent, const SpotTableCols& cols);  // vertical scroll only, 17-18 px rows
// Sets the row count to n and refreshes changed cells; the scroll position is kept.
void spotTableSet(lv_obj_t* table, size_t n, CellFn cell, void* ctx);
