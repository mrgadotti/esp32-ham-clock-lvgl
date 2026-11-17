#include "dx_screen.h"

#include "band_mode.h"
#include "spot_list_screen.h"

Screen* createDxScreen() {
  static const uint8_t kBands[] = {BAND_160M, BAND_80M, BAND_40M, BAND_30M, BAND_20M, BAND_17M,
                                   BAND_15M,  BAND_12M, BAND_10M, BAND_VHF, BAND_UHF, BAND_SHF};
  return createSpotListScreen({ScreenId::DxCluster, Tr::DxCluster, true, spotSrc(g_store.dx), kBands, sizeof kBands, &Config::s2_mode, &Config::s2_band, nullptr,
                              [] { g_store.dx.write([](RingBuffer<Spot, kDxSpots>& r) { r.clear(); }); }});
}
