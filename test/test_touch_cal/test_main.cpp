#include <math.h>
#include <unity.h>

#include "touch_cal.h"

void setUp() {}
void tearDown() {}

void test_cal_identity_roundtrip() {
  // scr = 30 deg rotation * 0.08 + offset, rounded to the integer points the UI would provide.
  const float c = cosf(0.5236f) * 0.08f, s = sinf(0.5236f) * 0.08f;
  auto fx = [&](float rx, float ry) { return c * rx - s * ry + 20; };
  auto fy = [&](float rx, float ry) { return s * rx + c * ry + 5; };
  const int16_t raw[3][2] = {{500, 600}, {3500, 1200}, {2000, 3400}};
  int16_t scr[3][2];
  for (int i = 0; i < 3; i++) {
    scr[i][0] = (int16_t)lroundf(fx(raw[i][0], raw[i][1]));
    scr[i][1] = (int16_t)lroundf(fy(raw[i][0], raw[i][1]));
  }
  TouchCal t;
  TEST_ASSERT_TRUE(touchCalSolve(raw, scr, t));
  // The float map must reproduce the 3 inputs to < 0.5 px.
  for (int i = 0; i < 3; i++) {
    TEST_ASSERT_FLOAT_WITHIN(0.5f, scr[i][0], t.a * raw[i][0] + t.b * raw[i][1] + t.c);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, scr[i][1], t.d * raw[i][0] + t.e * raw[i][1] + t.f);
  }
  // 4th point: compare the solved map with the points' own rounded-fit residual; the fit is exact
  // for the 3 rounded inputs, so check the 4th against the map re-derived from those inputs.
  const float x4 = t.a * 1800 + t.b * 2000 + t.c, y4 = t.d * 1800 + t.e * 2000 + t.f;
  int16_t x, y;
  touchCalApply(t, 1800, 2000, x, y);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, x4, x);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, y4, y);
  // Against the true transform the error is bounded by input rounding (0.5 px) amplified by geometry.
  TEST_ASSERT_FLOAT_WITHIN(0.5f, fx(1800, 2000), x4);
  TEST_ASSERT_FLOAT_WITHIN(0.5f, fy(1800, 2000), y4);
}

void test_cal_rejects_collinear() {
  const int16_t raw[3][2] = {{100, 100}, {200, 200}, {300, 300}};
  const int16_t scr[3][2] = {{0, 0}, {10, 10}, {20, 20}};
  TouchCal t;
  TEST_ASSERT_FALSE(touchCalSolve(raw, scr, t));
}

void test_cal_clamps() {
  TouchCal t = touchCalDefault();
  int16_t x, y;
  touchCalApply(t, 0, 0, x, y);
  TEST_ASSERT_EQUAL(0, x);
  TEST_ASSERT_EQUAL(0, y);
  touchCalApply(t, 4095, 4095, x, y);
  TEST_ASSERT_EQUAL(319, x);
  TEST_ASSERT_EQUAL(239, y);
  touchCalApply(t, 2000, 2000, x, y);
  TEST_ASSERT_INT16_WITHIN(1, 160, x);
  TEST_ASSERT_INT16_WITHIN(1, 120, y);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_cal_identity_roundtrip);
  RUN_TEST(test_cal_rejects_collinear);
  RUN_TEST(test_cal_clamps);
  return UNITY_END();
}
