#include "filter_menu.h"

#include "band_mode.h"
#include "i18n.h"
#include "../fonts/fonts.h"
#include "../theme.h"

namespace {
constexpr uint8_t kMaxBands = 16;
struct Menu {
  lv_obj_t* panel;
  uint8_t bands[kMaxBands];
  uint8_t nBands;
  uint16_t* bandOut;
  uint8_t* modeOut;
  uint16_t band;  // working copies
  uint8_t mode;
  void (*onSave)();
  lv_obj_t* modeTile[4];            // ALL, CW, SSB, DIGI
  lv_obj_t* bandTile[kMaxBands + 1];  // ALL + bands
};
Menu* m = nullptr;

void paint(lv_obj_t* tile, bool on) {
  lv_obj_set_style_bg_color(tile, on ? COL_ACCENT : lv_color_hex(0x808080), 0);
  lv_obj_set_style_text_color(tile, on ? COL_BG : COL_TEXT, 0);
}

void refresh() {
  for (int i = 0; i < 4; i++) paint(m->modeTile[i], i == 0 ? m->mode == 0 : (m->mode & (1 << (i - 1))));
  paint(m->bandTile[0], m->band == 0);
  for (uint8_t i = 0; i < m->nBands; i++) paint(m->bandTile[i + 1], m->band & (1u << m->bands[i]));
}

lv_obj_t* tile(lv_obj_t* p, const char* txt, int x, int y, int w, int h, lv_event_cb_t cb, intptr_t id) {
  lv_obj_t* t = lv_obj_create(p);
  lv_obj_remove_style_all(t);
  lv_obj_set_pos(t, x, y);
  lv_obj_set_size(t, w, h);
  lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(t, 3, 0);
  lv_obj_set_style_text_font(t, &font_14, 0);
  lv_obj_remove_flag(t, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* l = lv_label_create(t);
  lv_label_set_text(l, txt);
  lv_obj_center(l);
  lv_obj_add_flag(t, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(t, cb, LV_EVENT_CLICKED, (void*)id);
  return t;
}

void onMode(lv_event_t* e) {
  intptr_t i = (intptr_t)lv_event_get_user_data(e);
  if (i == 0) m->mode = 0;
  else m->mode ^= (1 << (i - 1));
  refresh();
}

void onBand(lv_event_t* e) {
  intptr_t i = (intptr_t)lv_event_get_user_data(e);
  if (i == 0) m->band = 0;
  else m->band ^= (1u << m->bands[i - 1]);
  refresh();
}

void finish(bool save) {
  void (*cb)() = nullptr;
  if (save) {
    *m->bandOut = m->band;
    *m->modeOut = m->mode;
    cb = m->onSave;
  }
  filterMenuClose();
  if (cb) cb();
}
void onOk(lv_event_t*) { finish(true); }
void onCancel(lv_event_t*) { finish(false); }

void label(lv_obj_t* p, const char* txt, int y) {
  lv_obj_t* l = lv_label_create(p);
  lv_obj_set_style_text_font(l, &font_14, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(0xC0C0C0), 0);
  lv_label_set_text(l, txt);
  lv_obj_set_pos(l, 8, y);
}
}  // namespace

bool filterMenuIsOpen() { return m != nullptr; }

void filterMenuClose() {
  if (!m) return;
  lv_obj_delete_async(m->panel);  // may run inside a tile's own click callback
  delete m;
  m = nullptr;
}

void filterMenuOpen(lv_obj_t*, const uint8_t* bands, uint8_t nBands, uint16_t* bandMask, uint8_t* modeMask,
                    void (*onSave)()) {
  if (m) return;
  m = new Menu{};
  m->nBands = nBands > kMaxBands ? kMaxBands : nBands;
  for (uint8_t i = 0; i < m->nBands; i++) m->bands[i] = bands[i];
  m->bandOut = bandMask;
  m->modeOut = modeMask;
  m->band = *bandMask;
  m->mode = *modeMask;
  m->onSave = onSave;

  lv_obj_t* p = m->panel = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(p);
  lv_obj_set_size(p, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(p, COL_BG, 0);
  lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
  lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(p, LV_OBJ_FLAG_CLICKABLE);  // swallow touches

  lv_obj_t* t = lv_label_create(p);
  lv_obj_set_style_text_font(t, &font_18, 0);
  lv_obj_set_style_text_color(t, COL_ACCENT, 0);
  lv_label_set_text(t, tr(Tr::Filters));
  lv_obj_set_pos(t, 8, 2);

  label(p, tr(Tr::Mode), 24);
  const char* names[4] = {tr(Tr::All), tr(Tr::FilterCw), tr(Tr::FilterSsb), tr(Tr::FilterDigi)};
  for (int i = 0; i < 4; i++) m->modeTile[i] = tile(p, names[i], 8 + i * 77, 42, 70, 24, onMode, i);

  label(p, tr(Tr::Band), 70);
  // 4 columns; ALL first, then the bands.
  for (uint8_t i = 0; i <= m->nBands; i++) {
    int x = 8 + (i % 4) * 78, y = 88 + (i / 4) * 28;
    m->bandTile[i] = tile(p, i == 0 ? tr(Tr::All) : bandName((Band)m->bands[i - 1]), x, y, 72, 24, onBand, i);
  }
  lv_obj_t* ok = tile(p, tr(Tr::Ok), 40, 208, 100, 26, onOk, 0);
  lv_obj_t* cancel = tile(p, tr(Tr::Cancel), 180, 208, 100, 26, onCancel, 0);
  paint(ok, true);
  paint(cancel, false);
  refresh();
}
