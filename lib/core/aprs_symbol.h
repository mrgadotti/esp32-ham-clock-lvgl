#pragma once
#include "i18n.h"
// APRS symbol {table, code} lookups; one table feeds the list short name and the overlay icon/description.
enum class AprsIcon : uint8_t { None, Car, House, Human };
// Short name for the list; false if the symbol is not in the table.
bool aprsSymbolName(const char sym[2], Tr& out);
// Overlay: drawn icon (None = use `desc` text). Unknown symbols give None + Tr::DescOther; any 'L' table gives DescLora.
void aprsSymbolInfo(const char sym[2], AprsIcon& icon, Tr& desc);
