#include "hamalert_screen.h"

#include "band_mode.h"
#include "spot_list_screen.h"

Screen* createHamAlertScreen() {
  static const uint8_t kBands[] = {BAND_160M, BAND_80M, BAND_40M, BAND_30M, BAND_20M, BAND_17M,
                                   BAND_15M,  BAND_12M, BAND_10M, BAND_VHF, BAND_UHF, BAND_SHF};
  return createSpotListScreen({ScreenId::HamAlert, Tr::HamAlert, &g_store.hamalert, kBands, sizeof kBands, &Config::s8_mode, &Config::s8_band});
}
