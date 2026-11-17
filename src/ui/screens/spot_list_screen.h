#pragma once
#include "../../app/data_store.h"
#include "../screen.h"
#include "band_mode.h"
#include "config.h"
#include "i18n.h"

// Type-erased view of a Guarded<RingBuffer<Spot, N>>: one screen class serves the DX ring and the 30-spot ones.
struct SpotSrc {
  const void* store;
  uint32_t (*version)(const void*);
  // Under the store lock: writes the store indexes that pass the band/mode masks to idx (at most cap), sets ver to the
  // version read in the same critical section and returns how many were written.
  size_t (*filter)(const void*, uint8_t* idx, size_t cap, uint16_t band, uint8_t mode, uint32_t& ver);
  void (*get)(const void*, size_t storeIdx, Spot& out);  // one lock + copy
};

constexpr size_t SPOT_LIST_MAX = kDxSpots;  // largest ring a SpotSrc may wrap

template <size_t N>
SpotSrc spotSrc(const Guarded<RingBuffer<Spot, N>>& g) {
  static_assert(N <= SPOT_LIST_MAX && N <= 255, "store indexes are uint8_t and idx_ is sized for the DX ring");
  using G = Guarded<RingBuffer<Spot, N>>;
  return {&g, [](const void* s) { return static_cast<const G*>(s)->version(); },
          [](const void* s, uint8_t* idx, size_t cap, uint16_t band, uint8_t mode, uint32_t& ver) {
            size_t n = 0;
            static_cast<const G*>(s)->read([&](const RingBuffer<Spot, N>& r) {
              ver = static_cast<const G*>(s)->version();
              for (size_t i = 0; i < r.size() && n < cap; i++)
                if (passesFilter((Band)r.at(i).band, (Mode)r.at(i).mode, band, mode)) idx[n++] = (uint8_t)i;
            });
            return n;
          },
          [](const void* s, size_t i, Spot& out) {
            static_cast<const G*>(s)->read([&](const RingBuffer<Spot, N>& r) { out = r.at(i); });
          }};
}

// DX, POTA and HamAlert share one screen: UTC/CALL/MHz/MOD + (comment | country) table plus a band/mode filter.
struct SpotListDef {
  ScreenId id;
  Tr title;
  bool comment;  // last column: true = MSG (Spot::comment), false = COUNTRY
  SpotSrc src;
  const uint8_t* bands;
  uint8_t nBands;
  int32_t Config::*modeKey;  // saved filter masks
  int32_t Config::*bandKey;
};
Screen* createSpotListScreen(const SpotListDef& def);
