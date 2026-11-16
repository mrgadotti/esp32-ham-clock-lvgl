#include <unity.h>
#include <math.h>
#include "radar_proj.h"
void setUp() {} void tearDown() {}
void test_project_east() {
  int16_t x, y; TEST_ASSERT_TRUE(radarProject({1, 0, 0}, 25, 90, 50, 100, 160, 128, x, y));
  TEST_ASSERT_EQUAL(210, x); TEST_ASSERT_EQUAL(128, y); }
void test_project_north() {
  int16_t x, y; TEST_ASSERT_TRUE(radarProject({1, 0, 0}, 25, 0, 50, 100, 160, 128, x, y));
  TEST_ASSERT_EQUAL(160, x); TEST_ASSERT_EQUAL(78, y); }
void test_zoom_recenters() {
  RadarView v = radarZoomAt(radarReset(), 210, 128, 50, 100, 160, 128);
  TEST_ASSERT_EQUAL_FLOAT(2, v.zoom);
  int16_t x, y; TEST_ASSERT_TRUE(radarProject(v, 25, 90, 50, 100, 160, 128, x, y));
  TEST_ASSERT_EQUAL(160, x); TEST_ASSERT_EQUAL(128, y); }
void test_zoom_caps_at_8() {
  RadarView v = radarReset();
  for (int i = 0; i < 5; i++) v = radarZoomAt(v, 160, 128, 50, 100, 160, 128);
  TEST_ASSERT_EQUAL_FLOAT(8, v.zoom); }
void test_outside_returns_false() {
  int16_t x, y; TEST_ASSERT_FALSE(radarProject({1, 0, 0}, 200, 90, 50, 100, 160, 128, x, y)); }
void test_zoom_tap_north_sets_cy() {
  RadarView v = radarZoomAt(radarReset(), 160, 78, 50, 100, 160, 128);
  TEST_ASSERT_EQUAL_FLOAT(25, v.cyKm); TEST_ASSERT_EQUAL_FLOAT(0, v.cxKm); }
void test_project_west_south() {
  int16_t x, y; TEST_ASSERT_TRUE(radarProject({1, 0, 0}, 25, 270, 50, 100, 160, 128, x, y));
  TEST_ASSERT_EQUAL(110, x); TEST_ASSERT_EQUAL(128, y);
  TEST_ASSERT_TRUE(radarProject({1, 0, 0}, 25, 180, 50, 100, 160, 128, x, y));
  TEST_ASSERT_EQUAL(160, x); TEST_ASSERT_EQUAL(178, y); }
void test_invalid_inputs() {
  int16_t x, y; RadarView z = {0, 0, 0};
  TEST_ASSERT_FALSE(radarProject(z, 25, 90, 50, 100, 160, 128, x, y));
  TEST_ASSERT_FALSE(radarProject({1, 0, 0}, 25, 90, 0, 100, 160, 128, x, y));
  TEST_ASSERT_FALSE(radarProject({1, 0, 0}, NAN, 90, 50, 100, 160, 128, x, y));
  RadarView v = radarZoomAt({2, 3, 4}, 200, 100, 0, 100, 160, 128);
  TEST_ASSERT_EQUAL_FLOAT(2, v.zoom); TEST_ASSERT_EQUAL_FLOAT(3, v.cxKm); TEST_ASSERT_EQUAL_FLOAT(4, v.cyKm); }
int main() { UNITY_BEGIN(); RUN_TEST(test_project_east); RUN_TEST(test_project_north); RUN_TEST(test_zoom_recenters);
  RUN_TEST(test_zoom_caps_at_8); RUN_TEST(test_outside_returns_false);
  RUN_TEST(test_zoom_tap_north_sets_cy); RUN_TEST(test_project_west_south); RUN_TEST(test_invalid_inputs); return UNITY_END(); }
