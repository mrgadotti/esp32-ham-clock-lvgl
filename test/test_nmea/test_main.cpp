#include <unity.h>
#include <string.h>
#include "nmea.h"
void setUp() {} void tearDown() {}
static const char* RMC = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";
void test_rmc() {
  TEST_ASSERT_TRUE(nmeaChecksumOk(RMC));
  GpsState g; memset(&g, 0, sizeof g);
  TEST_ASSERT_TRUE(nmeaParse(RMC, g, 1234));
  TEST_ASSERT_TRUE(g.fix); TEST_ASSERT_TRUE(g.active);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 48.1173, g.lat);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 11.516667, g.lon);
  TEST_ASSERT_EQUAL_UINT32(123519, g.utc); TEST_ASSERT_EQUAL_UINT32(230394, g.date);
  TEST_ASSERT_EQUAL_UINT32(1234, g.lastFixMs);
  TEST_ASSERT_FLOAT_WITHIN(0.01, 41.48, g.speedKmh);
}
void test_bad_checksum() {
  char b[100]; strcpy(b, RMC); b[strlen(b) - 1] = '7';
  GpsState g; memset(&g, 0, sizeof g);
  TEST_ASSERT_FALSE(nmeaChecksumOk(b)); TEST_ASSERT_FALSE(nmeaParse(b, g, 0)); TEST_ASSERT_FALSE(g.fix);
}
void test_gga_sats() {
  const char* s = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
  GpsState g; memset(&g, 0, sizeof g);
  TEST_ASSERT_TRUE(nmeaParse(s, g, 5));
  TEST_ASSERT_EQUAL_UINT8(8, g.sats); TEST_ASSERT_TRUE(g.fix);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 48.1173, g.lat);
}
void test_gsv_and_frame() {
  GpsState g; memset(&g, 0, sizeof g);
  TEST_ASSERT_TRUE(nmeaParse("$GLGSV,2,1,07", g, 0)); TEST_ASSERT_EQUAL_UINT8(7, g.satsGlo);
  TEST_ASSERT_TRUE(nmeaParse("$GPSESP32[1,1,9,52.25,21.0,12:34:56,15-03-2026]", g, 9));
  TEST_ASSERT_TRUE(g.fix); TEST_ASSERT_EQUAL_UINT8(9, g.sats);
  TEST_ASSERT_EQUAL_UINT32(123456, g.utc); TEST_ASSERT_EQUAL_UINT32(150326, g.date);
  TEST_ASSERT_FLOAT_WITHIN(1e-4, 52.25, g.lat);
}
void test_nmea_rejects_garbage() {
  GpsState g; memset(&g, 0, sizeof g);
  TEST_ASSERT_FALSE(nmeaParse("", g, 0)); TEST_ASSERT_FALSE(nmeaParse("hello world", g, 0));
  TEST_ASSERT_FALSE(nmeaParse(nullptr, g, 0)); TEST_ASSERT_FALSE(nmeaParse("$GPSESP32[1,2]", g, 0));
  TEST_ASSERT_FALSE(nmeaParse("$GPRMC,12*", g, 0));
  TEST_ASSERT_FALSE(g.active);
}
void test_gn_talker() {
  GpsState g; memset(&g, 0, sizeof g);
  TEST_ASSERT_TRUE(nmeaParse("$GNRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W", g, 1));
  TEST_ASSERT_TRUE(g.fix);
  memset(&g, 0, sizeof g);
  TEST_ASSERT_TRUE(nmeaParse("$GNGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,", g, 1));
  TEST_ASSERT_TRUE(g.fix); TEST_ASSERT_EQUAL_UINT8(8, g.sats);
}
void test_fix_validity() {
  GpsState g; memset(&g, 0, sizeof g);
  TEST_ASSERT_TRUE(nmeaParse("$GPRMC,123519,V,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W", g, 1));
  TEST_ASSERT_FALSE(g.fix);
  TEST_ASSERT_TRUE(nmeaParse("$GPRMC,123519,A,,,,,022.4,084.4,230394,003.1,W", g, 1));
  TEST_ASSERT_FALSE(g.fix);
  TEST_ASSERT_TRUE(nmeaParse("$GPRMC,123519,A,9100.000,N,01131.000,E,022.4,084.4,230394,003.1,W", g, 1));
  TEST_ASSERT_FALSE(g.fix);  // latitude out of range
}
void test_frame_invalid_coords_untouched() {
  GpsState g; memset(&g, 0, sizeof g); g.sats = 3;
  TEST_ASSERT_FALSE(nmeaParse("$GPSESP32[1,1,9,95.0,21.0,12:34:56,15-03-2026]", g, 1));
  TEST_ASSERT_FALSE(g.active); TEST_ASSERT_EQUAL_UINT8(3, g.sats);
  TEST_ASSERT_TRUE(nmeaParse("$GPSESP32[ 1 , 1 , 9 ,52.25, 21.0 ,12:34:56,15-03-2026]", g, 1));
  TEST_ASSERT_TRUE(g.fix);
}
int main() { UNITY_BEGIN(); RUN_TEST(test_rmc); RUN_TEST(test_bad_checksum); RUN_TEST(test_gga_sats);
  RUN_TEST(test_gsv_and_frame); RUN_TEST(test_nmea_rejects_garbage);
  RUN_TEST(test_gn_talker); RUN_TEST(test_fix_validity); RUN_TEST(test_frame_invalid_coords_untouched); return UNITY_END(); }
