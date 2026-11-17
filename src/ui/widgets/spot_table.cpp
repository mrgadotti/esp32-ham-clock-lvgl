#include "spot_table.h"

#include <string.h>

#include "../fonts/fonts.h"
#include "../theme.h"
#include "header.h"
#include "ui_kit.h"

namespace {
struct State {
  SpotTableCols cols;
};

// Zebra stripe: odd rows get a gray fill (original 0x0841 is ~#080808, invisible on the panel).
void stripeRow(lv_draw_task_t* t, const lv_draw_dsc_base_t* base) {
  lv_draw_fill_dsc_t* fd = lv_draw_task_get_fill_dsc(t);
  if (!fd) return;
  fd->opa = (base->id1 & 1) ? LV_OPA_COVER : LV_OPA_TRANSP;
  fd->color = lv_color_hex(0x2A2A2A);
}

void onDraw(lv_event_t* e) {
  lv_draw_task_t* t = lv_event_get_draw_task(e);
  auto* base = static_cast<lv_draw_dsc_base_t*>(lv_draw_task_get_draw_dsc(t));
  if (base->part != LV_PART_ITEMS) return;
  stripeRow(t, base);
  lv_draw_label_dsc_t* ld = lv_draw_task_get_label_dsc(t);
  if (!ld) return;
  auto* st = static_cast<State*>(lv_obj_get_user_data(lv_event_get_target_obj(e)));
  if (st && base->id2 < st->cols.n) ld->color = st->cols.color[base->id2];
}

void onDelete(lv_event_t* e) {
  delete static_cast<State*>(lv_obj_get_user_data(lv_event_get_target_obj(e)));
}
}  // namespace

lv_obj_t* spotTableCreate(lv_obj_t* parent, const SpotTableCols& cols) {
  lv_obj_t* t = lv_table_create(parent);
  lv_obj_remove_style_all(t);
  lv_obj_set_style_text_font(t, &font_14, LV_PART_ITEMS);
  lv_obj_set_style_text_color(t, COL_TEXT, LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(t, LV_OPA_COVER, LV_PART_ITEMS);  // fill task must exist; stripeRow sets real opa
  lv_obj_set_style_border_width(t, 0, LV_PART_ITEMS);
  lv_obj_set_style_pad_all(t, 0, LV_PART_ITEMS);
  lv_obj_set_style_pad_left(t, SPOT_TABLE_PAD_L, LV_PART_ITEMS);
  lv_obj_set_scroll_dir(t, LV_DIR_VER);
  lv_obj_add_flag(t, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
  int w = 0;
  lv_table_set_column_count(t, cols.n);
  for (uint8_t c = 0; c < cols.n; c++) {
    lv_table_set_column_width(t, c, cols.width[c]);
    w += cols.width[c];
  }
  lv_obj_set_width(t, w);
  // Header separator (original: 1 px muted gray under the column titles); sits in the 18 px header band above the table.
  lv_obj_t* sep = lv_obj_create(parent);
  lv_obj_remove_style_all(sep);
  lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(sep, lv_color_hex(0xD3D3D3), 0);  // TFT_LIGHTGREY (dark-theme muted)
  lv_obj_set_size(sep, 320, 1);
  lv_obj_set_pos(sep, 0, HEADER_H + 17);
  lv_obj_set_user_data(t, new State{cols});
  lv_obj_add_event_cb(t, onDraw, LV_EVENT_DRAW_TASK_ADDED, nullptr);
  lv_obj_add_event_cb(t, onDelete, LV_EVENT_DELETE, nullptr);
  return t;
}

void spotTableSet(lv_obj_t* t, size_t n, CellFn cell, void* ctx) {
  auto* st = static_cast<State*>(lv_obj_get_user_data(t));
  if (!st) return;
  int32_t sy = lv_obj_get_scroll_y(t);
  uint32_t oldRows = lv_table_get_row_count(t);
  lv_table_set_row_count(t, n);
  char buf[48];
  for (size_t r = 0; r < n; r++)
    for (uint8_t c = 0; c < st->cols.n; c++) {
      buf[0] = 0;
      if (!cell(r, c, buf, sizeof buf, ctx)) buf[0] = 0;
      const char* old = lv_table_get_cell_value(t, r, c);
      if (!old || strcmp(old, buf) != 0) lv_table_set_cell_value(t, r, c, buf);  // keeps the cell's ctrl bits
      if (r >= oldRows) lv_table_set_cell_ctrl(t, r, c, LV_TABLE_CELL_CTRL_TEXT_CROP);  // only new cells
    }
  lv_obj_scroll_to_y(t, sy, LV_ANIM_OFF);
}

SpotListUi spotListUiCreate(lv_obj_t* p, Tr title, const SpotTableCols& cols, const Tr* colTitles, Tr waitText,
                            lv_event_cb_t onMenu) {
  SpotListUi ui;
  ui.hdr = headerCreate(p, title);
  uiHamburger(ui.hdr, onMenu);
  int x = 0;
  for (uint8_t i = 0; i < cols.n; i++) {
    uiLabel(p, &font_14, lv_color_hex(0xA0A0A0), tr(colTitles[i]), x + SPOT_TABLE_PAD_L, HEADER_H);
    x += cols.width[i];
  }
  ui.table = spotTableCreate(p, cols);
  lv_obj_set_pos(ui.table, 0, HEADER_H + 18);
  lv_obj_set_height(ui.table, 240 - HEADER_H - 18);
  ui.wait = uiLabel(p, &font_18, COL_ERROR, tr(waitText));
  lv_obj_center(ui.wait);
  return ui;
}

void spotListUiSet(const SpotListUi& ui, size_t n, CellFn cell, void* ctx) {
  lv_obj_set_flag(ui.wait, LV_OBJ_FLAG_HIDDEN, n != 0);
  lv_obj_set_flag(ui.table, LV_OBJ_FLAG_HIDDEN, n == 0);
  spotTableSet(ui.table, n, cell, ctx);
}
