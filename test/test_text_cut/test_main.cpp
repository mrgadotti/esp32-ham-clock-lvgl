#include <unity.h>
#include <string.h>
#include "text_cut.h"

void setUp() {}
void tearDown() {}

void test_short_text_unchanged() {
  char b[16];
  textCut(b, sizeof b, "FT8 CQ", 15);
  TEST_ASSERT_EQUAL_STRING("FT8 CQ", b);
}
void test_ascii_cut() {
  char b[16];
  textCut(b, sizeof b, "0123456789ABCDEFGHIJ", 15);
  TEST_ASSERT_EQUAL_STRING("0123456789ABCDE", b);
}
void test_utf8_not_split() {
  char b[16];
  // 14 ASCII + "e acute" (C3 A9): a 15-byte cut would keep C3 only; it must drop it.
  textCut(b, sizeof b, "01234567890123\xC3\xA9xyz", 15);
  TEST_ASSERT_EQUAL_STRING("01234567890123", b);
  textCut(b, sizeof b, "0123456789012\xC3\xA9xyz", 15);  // the character fits exactly (bytes 13-14)
  TEST_ASSERT_EQUAL_STRING("0123456789012\xC3\xA9", b);
}
void test_small_buffer() {
  char b[4];
  textCut(b, sizeof b, "ab\xC3\xA9z", 15);  // cap 4: "ab" + the 2-byte character would need 5
  TEST_ASSERT_EQUAL_STRING("ab", b);
}
void test_km_cell() {
  char b[8];
  formatKmCell(b, sizeof b, 0); TEST_ASSERT_EQUAL_STRING("0", b);
  formatKmCell(b, sizeof b, 9999); TEST_ASSERT_EQUAL_STRING("9999", b);
  formatKmCell(b, sizeof b, 10000); TEST_ASSERT_EQUAL_STRING("10k", b);
  formatKmCell(b, sizeof b, 20015); TEST_ASSERT_EQUAL_STRING("20k", b);
  formatKmCell(b, sizeof b, -5); TEST_ASSERT_EQUAL_STRING("0", b);
}

void test_hm_cell() {
  char b[8];
  formatHmCell(b, sizeof b, 0); TEST_ASSERT_EQUAL_STRING("--", b);
  formatHmCell(b, sizeof b, 86400 + 3600 * 7 + 60 * 5 + 59); TEST_ASSERT_EQUAL_STRING("07:05", b);
  formatHmCell(b, sizeof b, 3600 * 23 + 60 * 59); TEST_ASSERT_EQUAL_STRING("23:59", b);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_short_text_unchanged);
  RUN_TEST(test_ascii_cut);
  RUN_TEST(test_utf8_not_split);
  RUN_TEST(test_small_buffer);
  RUN_TEST(test_km_cell);
  RUN_TEST(test_hm_cell);
  return UNITY_END();
}
