#pragma once
#include "../../app/data_store.h"
#include "../screen.h"
#include "config.h"
#include "i18n.h"

// POTA and HamAlert share one screen: UTC/CALL/MHz/MODE/COUNTRY table over a 30-spot store plus a band/mode filter.
struct SpotListDef {
  ScreenId id;
  Tr title;
  Guarded<RingBuffer<Spot, 30>>* store;
  const uint8_t* bands;
  uint8_t nBands;
  int32_t Config::*modeKey;  // saved filter masks
  int32_t Config::*bandKey;
};
Screen* createSpotListScreen(const SpotListDef& def);
