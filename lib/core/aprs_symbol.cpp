#include "aprs_symbol.h"

namespace {
struct E { char t, c; Tr name; Tr desc; AprsIcon icon; };
// Order as the original list table; desc DescOther = no overlay-specific text.
const E k[] = {
    {'/', '-', Tr::SymHouse, Tr::DescOther, AprsIcon::House}, {'/', '>', Tr::SymCar, Tr::DescOther, AprsIcon::Car},
    {'/', '#', Tr::SymDigi, Tr::DescDigi, AprsIcon::Digi}, {'/', 'r', Tr::SymRepeater, Tr::DescRepeater, AprsIcon::Repeater},
    {'/', '[', Tr::SymHuman, Tr::DescOther, AprsIcon::Human}, {'/', 'l', Tr::SymLaptop, Tr::DescLaptop, AprsIcon::Laptop},
    {'/', 'L', Tr::SymLogin, Tr::DescOther, AprsIcon::None}, {'/', '_', Tr::SymWx, Tr::DescWx, AprsIcon::Wx},
    {'/', 's', Tr::SymShip, Tr::DescShip, AprsIcon::Ship}, {'/', 'a', Tr::SymAmbulance, Tr::DescAmbulance, AprsIcon::Ambulance},
    {'/', 'b', Tr::SymBike, Tr::DescBike, AprsIcon::Bike}, {'/', 'c', Tr::SymIcp, Tr::DescOther, AprsIcon::Icp},
    {'/', 'd', Tr::SymFire, Tr::DescFire, AprsIcon::Fire}, {'/', 'f', Tr::SymFiretruck, Tr::DescOther, AprsIcon::Firetruck},
    {'/', 'h', Tr::SymHospital, Tr::DescHospital, AprsIcon::Hospital}, {'/', 'y', Tr::SymYagi, Tr::DescYagi, AprsIcon::Yagi},
    {'/', 'g', Tr::SymGlider, Tr::DescGlider, AprsIcon::Glider}, {'/', 'k', Tr::SymTruck, Tr::DescTruck, AprsIcon::Truck},
    {'/', 'u', Tr::SymTruck, Tr::DescTruck, AprsIcon::BigTruck}, {'/', 'v', Tr::SymVan, Tr::DescVan, AprsIcon::Van},
    {'/', 'V', Tr::SymHamClock, Tr::DescOther, AprsIcon::HamClock}, {'/', 'p', Tr::SymRover, Tr::DescRover, AprsIcon::Rover},
    {'/', 'o', Tr::SymEoc, Tr::DescOther, AprsIcon::Eoc}, {'/', 't', Tr::SymTruckStop, Tr::DescOther, AprsIcon::TruckStop},
    {'/', 'm', Tr::SymMicERepeater, Tr::DescOther, AprsIcon::MicERepeater}, {'/', 'n', Tr::SymNode, Tr::DescNode, AprsIcon::Node},
    {'/', 'q', Tr::SymGrid, Tr::DescOther, AprsIcon::Grid},
    {'L', 'a', Tr::SymLora, Tr::DescLora, AprsIcon::Lora}, {'L', '_', Tr::SymLora, Tr::DescLora, AprsIcon::Lora},
    {'L', '#', Tr::SymLora, Tr::DescLora, AprsIcon::Lora}, {'L', '&', Tr::SymLora, Tr::DescLora, AprsIcon::Lora}};

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
  const E* e = find(sym);
  if (e) { icon = e->icon; desc = e->desc; }
  else if (sym[0] == 'L') desc = Tr::DescLora;
}
