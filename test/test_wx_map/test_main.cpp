#include <unity.h>
#include "wx_map.h"
void setUp() {} void tearDown() {}
void test_icon_ranges() {
  TEST_ASSERT_EQUAL(WxIcon::I200, wxIconForId(200, false)); TEST_ASSERT_EQUAL(WxIcon::I200, wxIconForId(232, true));
  TEST_ASSERT_EQUAL(WxIcon::I300, wxIconForId(300, false)); TEST_ASSERT_EQUAL(WxIcon::I300, wxIconForId(321, false));
  TEST_ASSERT_EQUAL(WxIcon::I500, wxIconForId(500, false)); TEST_ASSERT_EQUAL(WxIcon::I500, wxIconForId(501, false));
  TEST_ASSERT_EQUAL(WxIcon::I502, wxIconForId(502, false)); TEST_ASSERT_EQUAL(WxIcon::I502, wxIconForId(503, false));
  TEST_ASSERT_EQUAL(WxIcon::I511, wxIconForId(511, false)); TEST_ASSERT_EQUAL(WxIcon::I511, wxIconForId(531, false));
  TEST_ASSERT_EQUAL(WxIcon::I520, wxIconForId(504, false)); TEST_ASSERT_EQUAL(WxIcon::I520, wxIconForId(520, false));
  TEST_ASSERT_EQUAL(WxIcon::I600, wxIconForId(600, false)); TEST_ASSERT_EQUAL(WxIcon::I600, wxIconForId(602, false));
  TEST_ASSERT_EQUAL(WxIcon::I611, wxIconForId(611, false)); TEST_ASSERT_EQUAL(WxIcon::I611, wxIconForId(616, false));
  TEST_ASSERT_EQUAL(WxIcon::I600, wxIconForId(620, false)); TEST_ASSERT_EQUAL(WxIcon::I600, wxIconForId(622, false));
  TEST_ASSERT_EQUAL(WxIcon::I700, wxIconForId(701, false)); TEST_ASSERT_EQUAL(WxIcon::I700, wxIconForId(781, false));
  TEST_ASSERT_EQUAL(WxIcon::I803, wxIconForId(803, true)); TEST_ASSERT_EQUAL(WxIcon::I803, wxIconForId(804, false)); }
void test_icon_night_and_unknown() {
  TEST_ASSERT_EQUAL(WxIcon::I800, wxIconForId(800, false)); TEST_ASSERT_EQUAL(WxIcon::I800n, wxIconForId(800, true));
  TEST_ASSERT_EQUAL(WxIcon::I801, wxIconForId(801, false)); TEST_ASSERT_EQUAL(WxIcon::I801n, wxIconForId(802, true));
  int none[] = {0, -1, 199, 233, 299, 322, 499, 532, 599, 623, 699, 782, 799, 805, 99999};
  for (int id : none) TEST_ASSERT_EQUAL(WxIcon::None, wxIconForId(id, false)); }
void test_wind_level() {
  TEST_ASSERT_EQUAL(0, wxWindLevel(0)); TEST_ASSERT_EQUAL(0, wxWindLevel(4.99f)); TEST_ASSERT_EQUAL(1, wxWindLevel(5));
  TEST_ASSERT_EQUAL(1, wxWindLevel(9.99f)); TEST_ASSERT_EQUAL(2, wxWindLevel(10)); TEST_ASSERT_EQUAL(2, wxWindLevel(14.99f));
  TEST_ASSERT_EQUAL(3, wxWindLevel(15)); TEST_ASSERT_EQUAL(3, wxWindLevel(40)); }
void test_pm_level() {  // PM2.5 thresholds 12/35/55/150 (limits inclusive)
  TEST_ASSERT_EQUAL(0, wxPmLevel(0, 12, 35, 55, 150)); TEST_ASSERT_EQUAL(0, wxPmLevel(12, 12, 35, 55, 150));
  TEST_ASSERT_EQUAL(1, wxPmLevel(12.1f, 12, 35, 55, 150)); TEST_ASSERT_EQUAL(1, wxPmLevel(35, 12, 35, 55, 150));
  TEST_ASSERT_EQUAL(2, wxPmLevel(55, 12, 35, 55, 150)); TEST_ASSERT_EQUAL(3, wxPmLevel(150, 12, 35, 55, 150));
  TEST_ASSERT_EQUAL(4, wxPmLevel(150.1f, 12, 35, 55, 150)); }
int main() { UNITY_BEGIN(); RUN_TEST(test_icon_ranges); RUN_TEST(test_icon_night_and_unknown); RUN_TEST(test_wind_level); RUN_TEST(test_pm_level); return UNITY_END(); }
