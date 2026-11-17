// Runs the real config_store.cpp against stub Preferences/FreeRTOS (see this directory).
#define BOARD_CYD  // the real include/board.h needs one
#include <unity.h>

#include "../../src/app/config_store.cpp"

void onConfigChanged() {}  // strong definition lives in events.cpp, not linked here

void setUp() {
  g_stubNvs.clear();
  g_stubPuts = 0;
  g_stubFailPuts = false;
  g_stubOnPut = nullptr;
  g_stubDeadlocks = 0;
  Config c;
  configStoreLoad(c);
}
void tearDown() {}

static uint32_t g_readsDuringSave;
static void readDuringPut() {  // would deadlock (stub counts it) if the update still held the mutex while saving
  cfgRead([](const Config&) {});
  g_readsDuringSave++;
}

void test_update_bumps_version_and_persists() {
  uint32_t v = cfgVersion();
  int tz = 5;  // captured value: no file-scope globals needed
  TEST_ASSERT_TRUE(cfgUpdate([tz](Config& c) { c.tft_backlight = tz * 10; }));
  TEST_ASSERT_EQUAL_UINT32(v + 1, cfgVersion());
  Config got;
  cfgGet(got);
  TEST_ASSERT_EQUAL_INT(50, got.tft_backlight);
  Config reloaded;
  configStoreLoad(reloaded);
  TEST_ASSERT_EQUAL_INT(50, reloaded.tft_backlight);
}

void test_update_clamps() {
  cfgUpdate([](Config& c) { c.tft_backlight = 0; });
  cfgRead([](const Config& c) { TEST_ASSERT_EQUAL_INT(5, c.tft_backlight); });
}

void test_mutex_free_while_saving() {
  g_readsDuringSave = 0;
  g_stubOnPut = readDuringPut;
  TEST_ASSERT_TRUE(cfgUpdate([](Config& c) { c.tft_sw_sec = 45; }));
  TEST_ASSERT_TRUE(g_readsDuringSave > 50);  // one per field saved
  TEST_ASSERT_EQUAL_INT(0, g_stubDeadlocks);
  g_readsDuringSave = 0;
  ApplyResult r;
  TEST_ASSERT_TRUE(cfgApplyJson("{\"tft_switch_time_sec\":50}", r));
  TEST_ASSERT_TRUE(g_readsDuringSave > 50);
  TEST_ASSERT_EQUAL_INT(0, g_stubDeadlocks);
}

void test_save_failure_keeps_ram_change() {
  g_stubFailPuts = true;
  uint32_t v = cfgVersion();
  TEST_ASSERT_FALSE(cfgUpdate([](Config& c) { c.tft_sw_sec = 77; }));
  TEST_ASSERT_EQUAL_UINT32(v + 1, cfgVersion());
  cfgRead([](const Config& c) { TEST_ASSERT_EQUAL_INT(77, c.tft_sw_sec); });
  ApplyResult r;
  TEST_ASSERT_FALSE(cfgApplyJson("{\"tft_switch_time_sec\":78}", r));
  TEST_ASSERT_EQUAL_STRING("nvs_save", r.error);
  cfgRead([](const Config& c) { TEST_ASSERT_EQUAL_INT(78, c.tft_sw_sec); });
}

void test_apply_json_version_only_on_success() {
  uint32_t v = cfgVersion();
  ApplyResult r;
  TEST_ASSERT_TRUE(cfgApplyJson("{\"user_callsign\":\"PY2XX\"}", r));
  TEST_ASSERT_EQUAL_UINT32(v + 1, cfgVersion());
  TEST_ASSERT_FALSE(cfgApplyJson("{\"user_callsign\":\"ZZ\",\"cluster_port\":\"bad\"}", r));
  TEST_ASSERT_EQUAL_UINT32(v + 1, cfgVersion());
  cfgRead([](const Config& c) { TEST_ASSERT_EQUAL_STRING("PY2XX", c.user_callsign); });
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_update_bumps_version_and_persists);
  RUN_TEST(test_update_clamps);
  RUN_TEST(test_mutex_free_while_saving);
  RUN_TEST(test_save_failure_keeps_ram_change);
  RUN_TEST(test_apply_json_version_only_on_success);
  return UNITY_END();
}
