#include "pota_screen.h"

#include "band_mode.h"
#include "spot_list_screen.h"

Screen* createPotaScreen() {
  static const uint8_t kBands[] = {BAND_160M, BAND_80M, BAND_40M, BAND_30M, BAND_20M, BAND_17M, BAND_15M, BAND_12M, BAND_10M};
  return createSpotListScreen({ScreenId::Pota, Tr::Pota, false, spotSrc(g_store.pota), kBands, sizeof kBands, &Config::s7_mode, &Config::s7_band});
}
