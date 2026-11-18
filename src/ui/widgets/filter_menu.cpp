#include "filter_menu.h"

#include "band_mode.h"
#include "i18n.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "ui_kit.h"

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
  void (*onExtra)();
  lv_obj_t* modeTile[4];            // ALL, CW, SSB, DIGI
  lv_obj_t* bandTile[kMaxBands + 1];  // ALL + bands
};
Menu* m = nullptr;

void refresh() {
  for (int i = 0; i < 4; i++) uiTilePaint(m->modeTile[i], i == 0 ? m->mode == 0 : (m->mode & (1 << (i - 1))));
  uiTilePaint(m->bandTile[0], m->band == 0);
  for (uint8_t i = 0; i < m->nBands; i++) uiTilePaint(m->bandTile[i + 1], m->band & (1u << m->bands[i]));
}

void onMode(lv_event_t* e) {
  if (!m) return;
  intptr_t i = (intptr_t)lv_event_get_user_data(e);
  if (i == 0) m->mode = 0;
  else m->mode ^= (1 << (i - 1));
  refresh();
}

void onBand(lv_event_t* e) {
  if (!m) return;
  intptr_t i = (intptr_t)lv_event_get_user_data(e);
  if (i == 0) m->band = 0;
  else m->band ^= (1u << m->bands[i - 1]);
  refresh();
}

void finish(bool save) {
  if (!m) return;
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
void onExtra(lv_event_t*) {
  if (!m) return;
  void (*cb)() = m->onExtra;
  filterMenuClose();
  if (cb) cb();
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
                    void (*onSave)(), const char* extraLabel, void (*onExtra_)()) {
  if (m) return;
  m = new Menu{};
  m->nBands = nBands > kMaxBands ? kMaxBands : nBands;
  for (uint8_t i = 0; i < m->nBands; i++) m->bands[i] = bands[i];
  m->bandOut = bandMask;
  m->modeOut = modeMask;
  m->band = *bandMask;
  m->mode = *modeMask;
  m->onSave = onSave;
  m->onExtra = onExtra_;

  lv_obj_t* p = m->panel = uiModalPanel();
  uiLabel(p, &font_18, COL_ACCENT, tr(Tr::Filters), 8, 2);

  if (extraLabel) uiTilePaint(uiTile(p, extraLabel, 164, 0, 148, 24, onExtra, 0), true);

  uiLabel(p, &font_14, lv_color_hex(0xC0C0C0), tr(Tr::Mode), 8, 24);
  const char* names[4] = {tr(Tr::All), tr(Tr::FilterCw), tr(Tr::FilterSsb), tr(Tr::FilterDigi)};
  for (int i = 0; i < 4; i++) m->modeTile[i] = uiTile(p, names[i], 8 + i * 77, 42, 70, 24, onMode, i);

  uiLabel(p, &font_14, lv_color_hex(0xC0C0C0), tr(Tr::Band), 8, 70);
  // 4 columns, 34 px row pitch (28 px tiles); scrolls when the bands do not fit above OK/Cancel.
  lv_obj_t* area = uiScrollArea(p, 0, 88, 320, 116);
  for (uint8_t i = 0; i <= m->nBands; i++) {
    int x = 8 + (i % 4) * 78, y = (i / 4) * 34;
    m->bandTile[i] = uiTile(area, i == 0 ? tr(Tr::All) : bandName((Band)m->bands[i - 1]), x, y, 72, 28, onBand, i);
  }
  lv_obj_t* ok = uiTile(p, tr(Tr::Ok), 40, 208, 100, 26, onOk, 0);
  lv_obj_t* cancel = uiTile(p, tr(Tr::Cancel), 180, 208, 100, 26, onCancel, 0);
  uiTilePaint(ok, true);
  uiTilePaint(cancel, true);
  refresh();
}
