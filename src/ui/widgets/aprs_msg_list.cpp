#include "aprs_msg_list.h"

#include <stdio.h>
#include "i18n.h"
#include "text_cut.h"
#include "../../app/data_store.h"
#include "../../app/events.h"
#include "../fonts/fonts.h"
#include "../overlays/alert_overlay.h"
#include "../theme.h"
#include "header.h"
#include "ui_kit.h"

namespace {
struct Panel {
  lv_obj_t* panel;
  void (*onExit)();
  size_t n;
  AprsMessage rows[kAprsMessages];  // heap: ~1.9 KB, kept off the UI task stack
};
Panel* p = nullptr;

void line(lv_layer_t* ly, int x1, int y1, int x2, int y2) {
  lv_draw_line_dsc_t d;
  lv_draw_line_dsc_init(&d);
  d.color = COL_BG;
  d.width = 2;
  d.round_start = d.round_end = 1;
  d.p1.x = x1; d.p1.y = y1; d.p2.x = x2; d.p2.y = y2;
  lv_draw_line(ly, &d);
}

void onBackDraw(lv_event_t* e) {
  lv_area_t a;
  lv_obj_get_coords(lv_event_get_target_obj(e), &a);
  lv_layer_t* ly = lv_event_get_layer(e);
  int cx = a.x1 + 25, cy = a.y1 + 13;
  line(ly, cx - 9, cy, cx + 9, cy);
  line(ly, cx - 9, cy, cx - 2, cy - 7);
  line(ly, cx - 9, cy, cx - 2, cy + 7);
}

void onXDraw(lv_event_t* e) {
  lv_area_t a;
  lv_obj_get_coords(lv_event_get_target_obj(e), &a);
  lv_layer_t* ly = lv_event_get_layer(e);
  int cx = a.x1 + 25, cy = a.y1 + 13;
  line(ly, cx - 6, cy - 6, cx + 6, cy + 6);
  line(ly, cx - 6, cy + 6, cx + 6, cy - 6);
}

void onBack(lv_event_t*) { aprsMsgListClose(); }
void onX(lv_event_t*) {
  if (!p) return;
  void (*cb)() = p->onExit;
  aprsMsgListClose();
  if (cb) cb();
}

void onRow(lv_event_t* e) {
  if (!p) return;
  size_t i = (size_t)(intptr_t)lv_event_get_user_data(e);
  if (i >= p->n) return;
  UiEvent ev{};
  ev.kind = EventKind::AprsMessage;
  ev.msg = p->rows[i];
  overlayShow(ev);
}

lv_obj_t* hit(lv_obj_t* parent, int x, lv_event_cb_t click, lv_event_cb_t draw) {
  lv_obj_t* o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_pos(o, x, 0);
  lv_obj_set_size(o, 50, HEADER_H);
  lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(o, click, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(o, draw, LV_EVENT_DRAW_MAIN, nullptr);
  uiPressFx(o);
  return o;
}
}  // namespace

bool aprsMsgListIsOpen() { return p != nullptr; }

void aprsMsgListClose() {
  if (!p) return;
  lv_obj_delete_async(p->panel);  // may run inside a click callback of one of its own children
  delete p;
  p = nullptr;
}

void aprsMsgListOpen(void (*onExit)()) {
  if (p) return;
  p = new Panel{};
  p->onExit = onExit;
  g_store.aprsMsg.read([&](const AprsMsgRing& r) {
    p->n = r.size();
    for (size_t i = 0; i < p->n; i++) p->rows[i] = r.at(i);
  });

  lv_obj_t* pn = p->panel = uiModalPanel();
  lv_obj_t* bar = lv_obj_create(pn);
  lv_obj_remove_style_all(bar);
  lv_obj_set_size(bar, SCREEN_W, HEADER_H);
  lv_obj_set_style_bg_color(bar, COL_ACCENT, 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(bar, COL_BG, 0);
  lv_obj_set_style_text_font(bar, &font_18, 0);
  lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  hit(bar, 0, onBack, onBackDraw);
  hit(bar, SCREEN_W - 50, onX, onXDraw);
  lv_obj_t* t = lv_label_create(bar);
  lv_label_set_text(t, tr(Tr::Messages));
  lv_obj_align(t, LV_ALIGN_LEFT_MID, 58, 0);

  lv_obj_t* area = uiScrollArea(pn, 0, HEADER_H, SCREEN_W, SCREEN_H - HEADER_H);
  if (p->n == 0) {
    uiLabel(area, &font_14, COL_GREY, tr(Tr::NoData), 0, 80, SCREEN_W, LV_TEXT_ALIGN_CENTER);
    return;
  }
  for (size_t i = 0; i < p->n; i++) {
    const AprsMessage& m = p->rows[i];
    lv_obj_t* row = lv_obj_create(area);
    lv_obj_remove_style_all(row);
    lv_obj_set_pos(row, 0, (int)i * 49);
    lv_obj_set_size(row, SCREEN_W, 48);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, onRow, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    uiPressFx(row);

    char tm[16];
    formatHmCell(tm, sizeof tm, m.utc, uiTimeOffsetSec());
    uiLabel(row, &font_14, COL_GREY, tm, 10, 4);
    uiLabel(row, &font_14, COL_TEXT, m.from, 70, 4);
    lv_obj_t* tx = uiLabel(row, &font_14, COL_CYAN, m.text, 10, 25, 300);
    lv_label_set_long_mode(tx, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_height(tx, 18);  // single line: DOTS truncates at the label height
  }
}
