#pragma once
#include <stddef.h>
#include <stdint.h>
#include "lvgl.h"
#include "i18n.h"

// Deviation from the brief: the widget is generic (Spot, AprsStation, ...) so POTA/HamAlert/APRS can reuse it.
// Instead of `rowAt(i, Spot&, ctx)` it asks for one cell text at a time; the callback takes the store lock itself.
constexpr uint8_t SPOT_TABLE_MAX_COLS = 6;
// Cell text starts 1 px right of the column edge (no right padding); tools/check_text_fit.py mirrors this.
constexpr int SPOT_TABLE_PAD_L = 1;
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

// The whole list-screen scaffold shared by the spot and APRS lists: header with hamburger, column titles,
// table and a centred "waiting" label shown while the table is empty.
struct SpotListUi {
  lv_obj_t *hdr, *table, *wait;
};
SpotListUi spotListUiCreate(lv_obj_t* parent, Tr title, const SpotTableCols& cols, const Tr* colTitles, Tr waitText,
                            lv_event_cb_t onMenu);
// spotTableSet plus the waiting label / table visibility for n rows.
void spotListUiSet(const SpotListUi& ui, size_t n, CellFn cell, void* ctx);
