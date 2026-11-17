#include <unity.h>
#include <stdio.h>
#include <string.h>
#include "boot_ring.h"

void setUp() {}
void tearDown() {}

void test_push_order_and_version() {
  BootLogRing r{};
  r.push("a");
  r.push("b");
  TEST_ASSERT_EQUAL(2, r.count);
  TEST_ASSERT_EQUAL(2, r.ver);
  TEST_ASSERT_EQUAL_STRING("a", r.at(0));
  TEST_ASSERT_EQUAL_STRING("b", r.at(1));
}
void test_wraps_dropping_oldest() {
  BootLogRing r{};
  char b[8];
  for (int i = 0; i < 13; i++) { snprintf(b, sizeof b, "L%d", i); r.push(b); }
  TEST_ASSERT_EQUAL(kBootLogLines, r.count);
  TEST_ASSERT_EQUAL_STRING("L3", r.at(0));
  TEST_ASSERT_EQUAL_STRING("L12", r.at(kBootLogLines - 1));
}
void test_truncates_and_cleans() {
  BootLogRing r{};
  r.push("0123456789012345678901234567890123456789ABC");
  TEST_ASSERT_EQUAL(kBootLogLen - 1, strlen(r.at(0)));
  r.push("x\ny");
  TEST_ASSERT_EQUAL_STRING("x y", r.at(1));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_push_order_and_version);
  RUN_TEST(test_wraps_dropping_oldest);
  RUN_TEST(test_truncates_and_cleans);
  return UNITY_END();
}
