#include <unity.h>
#include "aprs_wx.h"
void setUp() {} void tearDown() {}

void test_wx() {
  AprsWx w;
  TEST_ASSERT_TRUE(aprsDecodeWx("_220/004g005t077r000p000P000h50b09900", w));
  TEST_ASSERT_TRUE(w.hasWindDir); TEST_ASSERT_EQUAL(220, w.windDirDeg);
  TEST_ASSERT_FLOAT_WITHIN(0.1, 25.0, w.tempC);
  TEST_ASSERT_EQUAL(50, w.humidityPct);
  TEST_ASSERT_FLOAT_WITHIN(0.1, 990.0, w.pressureHpa);
  TEST_ASSERT_FLOAT_WITHIN(0.1, 6.4, w.windSpeedKmh);
}
void test_wx_none() {
  AprsWx w;
  TEST_ASSERT_FALSE(aprsDecodeWx("just a comment", w));
  TEST_ASSERT_FALSE(aprsDecodeWx("", w));
  TEST_ASSERT_FALSE(aprsDecodeWx(nullptr, w));
}
int main() { UNITY_BEGIN(); RUN_TEST(test_wx); RUN_TEST(test_wx_none); return UNITY_END(); }
