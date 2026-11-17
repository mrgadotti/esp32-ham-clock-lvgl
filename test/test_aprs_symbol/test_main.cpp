#include <unity.h>
#include "aprs_symbol.h"
void setUp() {} void tearDown() {}
void test_name() {
  Tr n = Tr::DescOther;
  TEST_ASSERT_TRUE(aprsSymbolName("/-", n)); TEST_ASSERT_EQUAL(Tr::SymHouse, n);
  TEST_ASSERT_TRUE(aprsSymbolName("/k", n)); TEST_ASSERT_EQUAL(Tr::SymTruck, n);
  TEST_ASSERT_TRUE(aprsSymbolName("/u", n)); TEST_ASSERT_EQUAL(Tr::SymTruck, n);
  TEST_ASSERT_TRUE(aprsSymbolName("/q", n)); TEST_ASSERT_EQUAL(Tr::SymGrid, n);
  TEST_ASSERT_TRUE(aprsSymbolName("L&", n)); TEST_ASSERT_EQUAL(Tr::SymLora, n);
  TEST_ASSERT_TRUE(aprsSymbolName("/c", n)); TEST_ASSERT_EQUAL(Tr::SymIcp, n);
  TEST_ASSERT_TRUE(aprsSymbolName("/f", n)); TEST_ASSERT_EQUAL(Tr::SymFiretruck, n);
  TEST_ASSERT_TRUE(aprsSymbolName("/V", n)); TEST_ASSERT_EQUAL(Tr::SymHamClock, n);
  n = Tr::DescOther;
  TEST_ASSERT_FALSE(aprsSymbolName("Lz", n)); TEST_ASSERT_FALSE(aprsSymbolName("/!", n)); TEST_ASSERT_FALSE(aprsSymbolName("\\>", n));
  TEST_ASSERT_EQUAL(Tr::DescOther, n); }
void test_info() {
  AprsIcon i; Tr d;
  aprsSymbolInfo("/>", i, d); TEST_ASSERT_EQUAL(AprsIcon::Car, i);
  aprsSymbolInfo("/-", i, d); TEST_ASSERT_EQUAL(AprsIcon::House, i);
  aprsSymbolInfo("/[", i, d); TEST_ASSERT_EQUAL(AprsIcon::Human, i);
  aprsSymbolInfo("/r", i, d); TEST_ASSERT_EQUAL(AprsIcon::None, i); TEST_ASSERT_EQUAL(Tr::DescRepeater, d);
  aprsSymbolInfo("/u", i, d); TEST_ASSERT_EQUAL(Tr::DescTruck, d);
  aprsSymbolInfo("Lx", i, d); TEST_ASSERT_EQUAL(AprsIcon::None, i); TEST_ASSERT_EQUAL(Tr::DescLora, d);  // any code in table L
  aprsSymbolInfo("/L", i, d); TEST_ASSERT_EQUAL(Tr::DescOther, d);  // login: list name only
  aprsSymbolInfo("/!", i, d); TEST_ASSERT_EQUAL(AprsIcon::None, i); TEST_ASSERT_EQUAL(Tr::DescOther, d);
  aprsSymbolInfo("\\>", i, d); TEST_ASSERT_EQUAL(AprsIcon::None, i); TEST_ASSERT_EQUAL(Tr::DescOther, d); }
int main() { UNITY_BEGIN(); RUN_TEST(test_name); RUN_TEST(test_info); return UNITY_END(); }
