#include <unity.h>
#include <string.h>
#include "url_encode.h"
void setUp() {} void tearDown() {}
void test_unreserved_passthrough() { char o[32]; urlEncode("Az09-_.~", o, sizeof o); TEST_ASSERT_EQUAL_STRING("Az09-_.~", o); }
void test_escapes() {
  char o[64]; urlEncode("a b&c=d;e/f%g+h", o, sizeof o); TEST_ASSERT_EQUAL_STRING("a%20b%26c%3Dd%3Be%2Ff%25g%2Bh", o);
  urlEncode("\xC3\xA7", o, sizeof o); TEST_ASSERT_EQUAL_STRING("%C3%A7", o);  // high bytes, uppercase hex
  urlEncode("", o, sizeof o); TEST_ASSERT_EQUAL_STRING("", o); }
void test_truncation_keeps_whole_escapes() {
  char o[8]; urlEncode("ab cd ef", o, sizeof o);
  TEST_ASSERT_EQUAL_STRING("ab%20", o);
  char z[1] = {'x'}; urlEncode("abc", z, 0); TEST_ASSERT_EQUAL('x', z[0]);  // cap 0 writes nothing
  urlEncode("abc", z, 1); TEST_ASSERT_EQUAL('\0', z[0]); }
int main() { UNITY_BEGIN(); RUN_TEST(test_unreserved_passthrough); RUN_TEST(test_escapes); RUN_TEST(test_truncation_keeps_whole_escapes); return UNITY_END(); }
