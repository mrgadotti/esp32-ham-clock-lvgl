#include "aprs_symbol.h"

namespace {
struct E { char t, c; Tr name; Tr desc; AprsIcon icon; };
// Order as the original list table; desc DescOther = no overlay-specific text.
const E k[] = {
    {'/', '-', Tr::SymHouse, Tr::DescOther, AprsIcon::House}, {'/', '>', Tr::SymCar, Tr::DescOther, AprsIcon::Car},
    {'/', '#', Tr::SymDigi, Tr::DescDigi, AprsIcon::None}, {'/', 'r', Tr::SymRepeater, Tr::DescRepeater, AprsIcon::None},
    {'/', '[', Tr::SymHuman, Tr::DescOther, AprsIcon::Human}, {'/', 'l', Tr::SymLaptop, Tr::DescLaptop, AprsIcon::None},
    {'/', 'L', Tr::SymLogin, Tr::DescOther, AprsIcon::None}, {'/', '_', Tr::SymWx, Tr::DescWx, AprsIcon::None},
    {'/', 's', Tr::SymShip, Tr::DescShip, AprsIcon::None}, {'/', 'a', Tr::SymAmbulance, Tr::DescAmbulance, AprsIcon::None},
    {'/', 'b', Tr::SymBike, Tr::DescBike, AprsIcon::None}, {'/', 'c', Tr::SymIcp, Tr::DescOther, AprsIcon::None},
    {'/', 'd', Tr::SymFire, Tr::DescFire, AprsIcon::None}, {'/', 'f', Tr::SymFiretruck, Tr::DescOther, AprsIcon::None},
    {'/', 'h', Tr::SymHospital, Tr::DescHospital, AprsIcon::None}, {'/', 'y', Tr::SymYagi, Tr::DescYagi, AprsIcon::None},
    {'/', 'g', Tr::SymGlider, Tr::DescGlider, AprsIcon::None}, {'/', 'k', Tr::SymTruck, Tr::DescTruck, AprsIcon::None},
    {'/', 'u', Tr::SymTruck, Tr::DescTruck, AprsIcon::None}, {'/', 'v', Tr::SymVan, Tr::DescVan, AprsIcon::None},
    {'/', 'V', Tr::SymHamClock, Tr::DescOther, AprsIcon::None}, {'/', 'p', Tr::SymRover, Tr::DescRover, AprsIcon::None},
    {'/', 'o', Tr::SymEoc, Tr::DescOther, AprsIcon::None}, {'/', 't', Tr::SymTruckStop, Tr::DescOther, AprsIcon::None},
    {'/', 'm', Tr::SymMicERepeater, Tr::DescOther, AprsIcon::None}, {'/', 'n', Tr::SymNode, Tr::DescNode, AprsIcon::None},
    {'/', 'q', Tr::SymGrid, Tr::DescOther, AprsIcon::None},
    {'L', 'a', Tr::SymLora, Tr::DescLora, AprsIcon::None}, {'L', '_', Tr::SymLora, Tr::DescLora, AprsIcon::None},
    {'L', '#', Tr::SymLora, Tr::DescLora, AprsIcon::None}, {'L', '&', Tr::SymLora, Tr::DescLora, AprsIcon::None}};

const E* find(const char s[2]) {
  for (const E& e : k)
    if (e.t == s[0] && e.c == s[1]) return &e;
  return nullptr;
}
}  // namespace

bool aprsSymbolName(const char sym[2], Tr& out) {
  const E* e = find(sym);
  if (e) out = e->name;
  return e != nullptr;
}

void aprsSymbolInfo(const char sym[2], AprsIcon& icon, Tr& desc) {
  icon = AprsIcon::None;
  desc = Tr::DescOther;
  if (sym[0] == 'L') { desc = Tr::DescLora; return; }
  const E* e = find(sym);
  if (e) { icon = e->icon; desc = e->desc; }
}
