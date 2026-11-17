#include "spot_table.h"

#include <stdlib.h>
#include <string.h>

#include "../fonts/fonts.h"
#include "../theme.h"
#include "header.h"
#include "ui_kit.h"

namespace {
// Not an lv_table: its draw loop creates a fill and a measured label per cell and every chunk of every scroll frame
// pays for it. This widget draws one stripe per odd row and one unmeasured label per cell, straight from a string pool.
struct State {
  SpotTableCols cols;
  lv_obj_t* spacer;  // invisible child that gives the scroll range its height
  char* pool = nullptr;
  uint16_t* off = nullptr;  // pool offset of each cell, row-major
  uint32_t poolLen = 0, rows = 0;
  int32_t rowH, width;
};

bool intersect(lv_area_t* r, const lv_area_t* a, const lv_area_t* b) {  // lv_area_intersect is private API
  r->x1 = LV_MAX(a->x1, b->x1);
  r->y1 = LV_MAX(a->y1, b->y1);
  r->x2 = LV_MIN(a->x2, b->x2);
  r->y2 = LV_MIN(a->y2, b->y2);
  return r->x1 <= r->x2 && r->y1 <= r->y2;
}

void onDraw(lv_event_t* e) {
  lv_obj_t* t = lv_event_get_current_target_obj(e);
  auto* st = static_cast<State*>(lv_obj_get_user_data(t));
  if (!st || !st->rows) return;
  lv_layer_t* layer = lv_event_get_layer(e);
  lv_area_t o, clip;
  lv_obj_get_coords(t, &o);
  if (!intersect(&clip, &o, &layer->_clip_area)) return;
  const lv_area_t clipOri = layer->_clip_area;
  int32_t y0 = o.y1 - lv_obj_get_scroll_y(t);
  uint32_t first = clip.y1 > y0 ? (uint32_t)((clip.y1 - y0) / st->rowH) : 0;
  lv_draw_fill_dsc_t fd;
  lv_draw_fill_dsc_init(&fd);
  fd.base.layer = layer;
  fd.color = lv_color_hex(0x2A2A2A);  // zebra: the original 0x0841 is ~#080808, invisible on the panel
  lv_draw_label_dsc_t ld;
  lv_draw_label_dsc_init(&ld);
  ld.base.layer = layer;
  ld.font = &font_14;
  ld.flag = LV_TEXT_FLAG_EXPAND;  // no wrapping; the cell clip crops the text
  for (uint32_t r = first; r < st->rows; r++) {
    lv_area_t cell = {o.x1, y0 + (int32_t)r * st->rowH, o.x1, 0};
    cell.y2 = cell.y1 + st->rowH - 1;
    if (cell.y1 > clip.y2) break;
    if (r & 1) {
      layer->_clip_area = clip;  // the previous row's label clip is still set; it would cut the stripe
      lv_area_t row = {o.x1, cell.y1, o.x1 + st->width - 1, cell.y2};
      lv_draw_fill(layer, &fd, &row);
    }
    for (uint8_t c = 0; c < st->cols.n; c++) {
      cell.x2 = cell.x1 + st->cols.width[c] - 1;
      const char* txt = st->pool + st->off[r * st->cols.n + c];
      if (*txt && intersect(&layer->_clip_area, &clip, &cell)) {
        ld.text = txt;
        ld.color = st->cols.color[c];
        lv_area_t ta = {cell.x1 + SPOT_TABLE_PAD_L, cell.y1, cell.x2, cell.y2};
        lv_draw_label(layer, &ld, &ta);
      }
      cell.x1 = cell.x2 + 1;
    }
  }
  layer->_clip_area = clipOri;
}

void onDelete(lv_event_t* e) {
  auto* st = static_cast<State*>(lv_obj_get_user_data(lv_event_get_target_obj(e)));
  if (st) free(st->pool), free(st->off);
  delete st;
}
}  // namespace

lv_obj_t* spotTableCreate(lv_obj_t* parent, const SpotTableCols& cols) {
  lv_obj_t* t = lv_obj_create(parent);
  lv_obj_remove_style_all(t);
  lv_obj_set_scroll_dir(t, LV_DIR_VER);
  auto* st = new State{cols};
  st->rowH = lv_font_get_line_height(&font_14);
  for (uint8_t c = 0; c < cols.n; c++) st->width += cols.width[c];
  lv_obj_set_width(t, st->width);
  st->spacer = lv_obj_create(t);
  lv_obj_remove_style_all(st->spacer);
  lv_obj_remove_flag(st->spacer, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(st->spacer, 1, 1);
  lv_obj_set_pos(st->spacer, 0, 0);
  // Header separator (original: 1 px muted gray under the column titles); sits in the 18 px header band above the table.
  lv_obj_t* sep = lv_obj_create(parent);
  lv_obj_remove_style_all(sep);
  lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(sep, lv_color_hex(0xD3D3D3), 0);  // TFT_LIGHTGREY (dark-theme muted)
  lv_obj_set_size(sep, 320, 1);
  lv_obj_set_pos(sep, 0, HEADER_H + 17);
  lv_obj_set_user_data(t, st);
  lv_obj_add_event_cb(t, onDraw, LV_EVENT_DRAW_MAIN, nullptr);
  lv_obj_add_event_cb(t, onDelete, LV_EVENT_DELETE, nullptr);
  return t;
}

void spotTableSet(lv_obj_t* t, size_t n, CellFn cell, void* ctx) {
  auto* st = static_cast<State*>(lv_obj_get_user_data(t));
  if (!st) return;
  const uint8_t nc = st->cols.n;
  size_t cap = n * nc * 12 + 16, len = 0;
  char* pool = static_cast<char*>(malloc(cap));
  auto* off = static_cast<uint16_t*>(malloc((n * nc + 1) * sizeof(uint16_t)));
  if (!pool || !off) { free(pool); free(off); return; }  // keep the old rows
  char buf[48];
  for (size_t i = 0; i < n * nc; i++) {
    buf[0] = 0;
    if (!cell(i / nc, i % nc, buf, sizeof buf, ctx)) buf[0] = 0;
    size_t l = strlen(buf) + 1;
    if (len + l > cap) {
      cap = cap * 2 + l;
      char* np = static_cast<char*>(realloc(pool, cap));
      if (!np) { free(pool); free(off); return; }
      pool = np;
    }
    memcpy(pool + len, buf, l);
    off[i] = (uint16_t)len;
    len += l;
  }
  bool same = n == st->rows && len == st->poolLen && memcmp(pool, st->pool, len) == 0;  // offsets follow from the text
  if (same) { free(pool); free(off); return; }  // nothing changed: no invalidate, no redraw
  free(st->pool);
  free(st->off);
  st->pool = pool;
  st->off = off;
  st->poolLen = len;
  st->rows = n;
  lv_obj_set_height(st->spacer, (int32_t)n * st->rowH);
  lv_obj_update_layout(t);
  int32_t over = lv_obj_get_scroll_bottom(t);
  if (over < 0) lv_obj_scroll_by(t, 0, -over, LV_ANIM_OFF);  // the list got shorter than the scroll position
  lv_obj_invalidate(t);
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
  lv_obj_set_width(ui.wait, 300);  // long texts (PT) wrap onto a second line instead of being clipped
  lv_obj_set_style_text_align(ui.wait, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(ui.wait);
  return ui;
}

void spotListUiSet(const SpotListUi& ui, size_t n, CellFn cell, void* ctx) {
  lv_obj_set_flag(ui.wait, LV_OBJ_FLAG_HIDDEN, n != 0);
  lv_obj_set_flag(ui.table, LV_OBJ_FLAG_HIDDEN, n == 0);
  spotTableSet(ui.table, n, cell, ctx);
}
