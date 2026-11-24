#include "aprs_is_screen.h"

#include <string.h>

#include "i18n.h"
#include "../../app/config_store.h"
#include "../fonts/fonts.h"
#include "../theme.h"
#include "../widgets/header.h"
#include "../widgets/ui_kit.h"

namespace {
// APRS-IS status screen: title "APRS-IS", then the callsign of the station connected to it.
class AprsIsScreen : public Screen {
 public:
  ScreenId id() const override { return ScreenId::AprsIs; }

  void create(lv_obj_t* p) override {
    headerCreate(p, Tr::AprsIs);
    lbl_ = uiLabel(p, &font_14, COL_GREY, tr(Tr::AprsIsSta), 15, 32);
    call_ = uiLabel(p, &font_18, COL_TEXT, "", 15, 54);
    refresh(true);
  }

  void tick(uint32_t) override { refresh(false); }

 private:
  lv_obj_t *lbl_ = nullptr, *call_ = nullptr;
  char cache_[13] = "";  // Config::aprs_callsign is 12 chars + NUL

  void refresh(bool force) {
    if (!call_) return;
    cfgRead([this, force](const Config& c) {
      const char* s = c.aprs_callsign[0] ? c.aprs_callsign : "-";
      if (force || strcmp(s, cache_) != 0) {
        strcpy(cache_, s);
        lv_label_set_text(call_, cache_);
      }
    });
  }
};
}  // namespace

Screen* createAprsIsScreen() { return new AprsIsScreen(); }
