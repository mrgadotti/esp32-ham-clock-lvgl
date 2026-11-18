#include <unity.h>
#include "radar_label.h"

void setUp() {}
void tearDown() {}

static void check(float v, const char* want) {
  char b[16];
  formatRadarKm(v, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING(want, b);
}
void test_radar_km_small() { check(0.5f, "0.50km"); check(1.25f, "1.25km"); }
void test_radar_km_mid() { check(12.5f, "12.5km"); check(10.0f, "10.0km"); }
void test_radar_km_large() { check(150.0f, "150km"); check(99.96f, "100.0km"); check(100.4f, "100km"); }

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_radar_km_small);
  RUN_TEST(test_radar_km_mid);
  RUN_TEST(test_radar_km_large);
  return UNITY_END();
}
