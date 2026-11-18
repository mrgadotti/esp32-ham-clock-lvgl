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
void test_zoom_steps_and_caps_at_80() {
  RadarView v = {1, 3, 4};
  const float up[] = {2, 4, 8, 10, 15, 20, 30, 40, 50, 60, 80, 80};
  for (float z : up) { v = radarZoomStep(v, 1); TEST_ASSERT_EQUAL_FLOAT(z, v.zoom); }
  const float dn[] = {60, 50, 40, 30, 20, 15, 10, 8, 4, 2, 1, 1};
  for (float z : dn) { v = radarZoomStep(v, -1); TEST_ASSERT_EQUAL_FLOAT(z, v.zoom); }
  TEST_ASSERT_EQUAL_FLOAT(3, v.cxKm); TEST_ASSERT_EQUAL_FLOAT(4, v.cyKm);
  int16_t x, y; TEST_ASSERT_TRUE(radarProject({50, 0, 0}, 0.02f, 90, 1, 100, 160, 128, x, y));  // 5000 px/km at 50x
  TEST_ASSERT_EQUAL(260, x);
  TEST_ASSERT_TRUE(radarProject({80, 0, 0}, 0.0125f, 90, 1, 100, 160, 128, x, y));  // 8000 px/km at 80x
  TEST_ASSERT_EQUAL(260, x);
  TEST_ASSERT_FALSE(radarProject({80, 0, 0}, 0.03f, 90, 1, 100, 160, 128, x, y)); }  // 400 px: off screen
void test_tap_zoom_recenters_and_caps_at_80() {
  RadarView v = radarZoomAt(radarReset(), 210, 128, 50, 100, 160, 128);
  TEST_ASSERT_EQUAL_FLOAT(2, v.zoom);
  int16_t x, y; TEST_ASSERT_TRUE(radarProject(v, 25, 90, 50, 100, 160, 128, x, y));
  TEST_ASSERT_EQUAL(160, x); TEST_ASSERT_EQUAL(128, y);
  v = radarZoomAt(radarReset(), 160, 78, 50, 100, 160, 128);
  TEST_ASSERT_EQUAL_FLOAT(25, v.cyKm); TEST_ASSERT_EQUAL_FLOAT(0, v.cxKm);
  const float want[] = {4, 8, 16, 32, 64, 80, 80};
  for (float z : want) { v = radarZoomAt(v, 160, 128, 50, 100, 160, 128); TEST_ASSERT_EQUAL_FLOAT(z, v.zoom); }
  TEST_ASSERT_EQUAL_FLOAT(25, v.cyKm);
  v = radarZoomAt({2, 3, 4}, 200, 100, 0, 100, 160, 128);
  TEST_ASSERT_EQUAL_FLOAT(2, v.zoom); TEST_ASSERT_EQUAL_FLOAT(3, v.cxKm); }
void test_project_radius_150() {
  int16_t x, y; TEST_ASSERT_TRUE(radarProject({1, 0, 0}, 150, 90, 150, 100, 160, 128, x, y));
  TEST_ASSERT_EQUAL(260, x);
  TEST_ASSERT_TRUE(radarProject({20, 0, 0}, 7.5f, 90, 150, 100, 160, 128, x, y));
  TEST_ASSERT_EQUAL(260, x); }
void test_outside_returns_false() {
  int16_t x, y; TEST_ASSERT_FALSE(radarProject({1, 0, 0}, 200, 90, 50, 100, 160, 128, x, y)); }
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
}
int main() { UNITY_BEGIN(); RUN_TEST(test_project_east); RUN_TEST(test_project_north); RUN_TEST(test_zoom_steps_and_caps_at_80); RUN_TEST(test_tap_zoom_recenters_and_caps_at_80); RUN_TEST(test_project_radius_150); RUN_TEST(test_outside_returns_false);
 RUN_TEST(test_project_west_south); RUN_TEST(test_invalid_inputs); return UNITY_END(); }
