#include <unity.h>
#include <stdio.h>
#include <string.h>
#include "bridge_proto.h"
void setUp() {} void tearDown() {}

void test_bridge_list() {
  BridgeFsm f;
  f.onLine("BT[LIST_BEGIN]:2");
  f.onLine("BT[NAME]:Radio One[ADR]AABBCCDDEEFF[BT]");
  f.onLine("BT[NAME]:Radio Two[ADR]11:22:33:44:55:66[BT]");
  f.onLine("BT[LIST_END]");
  TEST_ASSERT_EQUAL_INT(2, f.deviceCount);
  TEST_ASSERT_EQUAL_STRING("Radio One", f.devices[0].name);
  TEST_ASSERT_EQUAL_STRING("112233445566", f.devices[1].addr);
  TEST_ASSERT_FALSE(f.scanInProgress);
  TEST_ASSERT_EQUAL_INT(BR_LIST_BEGIN, bridgeParseLine("BT[LIST_BEGIN]:2"));
  TEST_ASSERT_EQUAL_INT(BR_NONE, bridgeParseLine("garbage"));
}

void test_bridge_btspp_retry() {
  BridgeFsm f; char c[BRIDGE_CMD_MAX];
  f.tick(100);
  f.onLine("[BT] AUTO CONNECT OK");
  uint32_t at[8]; int sent = 0; bool gaveUpEarly = false;
  for (uint32_t t = 100; t <= 10000; t += 50) {
    f.tick(t);
    while (f.popCommand(c, sizeof c)) { TEST_ASSERT_EQUAL_STRING("BTSPP_ON", c); at[sent++] = t; }
    if (f.lastError[0] && t < 6100) gaveUpEarly = true;  // give-up only after the 3rd retry window
  }
  TEST_ASSERT_EQUAL_INT(3, sent);
  TEST_ASSERT_UINT32_WITHIN(50, 100, at[0]);
  TEST_ASSERT_UINT32_WITHIN(50, 2100, at[1]);
  TEST_ASSERT_UINT32_WITHIN(50, 4100, at[2]);
  TEST_ASSERT_FALSE(gaveUpEarly);
  TEST_ASSERT_EQUAL_STRING("BTSPP_ON_NO_ACK", f.lastError);
}

void test_bridge_retry_stops_on_ack() {
  BridgeFsm f; char c[BRIDGE_CMD_MAX];
  f.tick(0); f.onLine("AUTO CONNECT OK"); f.tick(0);
  TEST_ASSERT_TRUE(f.popCommand(c, sizeof c));
  f.onLine("ACK BTSPP_ON");
  f.tick(2000); f.tick(4000);
  TEST_ASSERT_FALSE(f.popCommand(c, sizeof c));
}

void test_bridge_scan_and_boot() {
  BridgeFsm f; char c[BRIDGE_CMD_MAX];
  f.tick(1000); f.startScan();
  TEST_ASSERT_TRUE(f.popCommand(c, sizeof c)); TEST_ASSERT_EQUAL_STRING("BTSPP_OFF", c);
  f.onLine("ACK BTSPP_OFF");
  f.tick(1100);                                   // within 250 ms cooldown
  TEST_ASSERT_FALSE(f.popCommand(c, sizeof c));
  f.tick(1300);
  TEST_ASSERT_TRUE(f.popCommand(c, sizeof c)); TEST_ASSERT_EQUAL_STRING("BT SCAN", c);
  f.tick(59999); TEST_ASSERT_FALSE(f.popCommand(c, sizeof c));
  f.tick(60000); TEST_ASSERT_TRUE(f.popCommand(c, sizeof c)); TEST_ASSERT_EQUAL_STRING("GPS ON", c);
  f.tick(69999); TEST_ASSERT_FALSE(f.popCommand(c, sizeof c));
  f.tick(70000); TEST_ASSERT_TRUE(f.popCommand(c, sizeof c)); TEST_ASSERT_EQUAL_STRING("BTSPP_ON", c);
}

void test_bridge_pair_and_limits() {
  BridgeFsm f; char c[BRIDGE_CMD_MAX];
  TEST_ASSERT_FALSE(f.pair("x", "zz", ""));
  TEST_ASSERT_TRUE(f.pair("My[Radio]", "AA:BB:CC:DD:EE:FF", "1234"));
  TEST_ASSERT_TRUE(f.popCommand(c, sizeof c));
  TEST_ASSERT_EQUAL_STRING("BTCNT[NAME]MyRadio[ADR]AABBCCDDEEFF[PSWD]1234", c);
  f.onLine("BTCNT_OK");
  TEST_ASSERT_EQUAL_INT(1, f.pairResult);
  TEST_ASSERT_EQUAL_STRING("AABBCCDDEEFF", f.pairedAddr);
  for (int i = 0; i < 30; i++) {
    char l[80]; snprintf(l, sizeof l, "BT[NAME]:D%d[ADR]AABBCCDDEE%02X[BT]", i, i);
    f.onLine(l);
  }
  TEST_ASSERT_EQUAL_INT(BRIDGE_MAX_DEVICES, f.deviceCount);
}
void test_bridge_ignores_nmea_lines() {
  TEST_ASSERT_EQUAL_INT(BR_NONE, bridgeParseLine("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A")); }
int main() { UNITY_BEGIN(); RUN_TEST(test_bridge_ignores_nmea_lines); RUN_TEST(test_bridge_list); RUN_TEST(test_bridge_btspp_retry);
  RUN_TEST(test_bridge_retry_stops_on_ack); RUN_TEST(test_bridge_scan_and_boot); RUN_TEST(test_bridge_pair_and_limits); return UNITY_END(); }
