#include <unity.h>
#include <string.h>
#include "ax25_kiss.h"
void setUp() {} void tearDown() {}

void test_kiss_escape() {
  uint8_t out[64];
  const char info[] = {(char)0xC0, (char)0xDB, 0};
  size_t n = kissEncodeUiFrame("PY2XX-9", "APRS", nullptr, 0, info, out, sizeof out);
  TEST_ASSERT_TRUE(n > 0);
  TEST_ASSERT_EQUAL_HEX8(0xC0, out[0]);
  TEST_ASSERT_EQUAL_HEX8(0xC0, out[n - 1]);
  TEST_ASSERT_EQUAL_HEX8(0xDB, out[n - 5]); TEST_ASSERT_EQUAL_HEX8(0xDC, out[n - 4]);
  TEST_ASSERT_EQUAL_HEX8(0xDB, out[n - 3]); TEST_ASSERT_EQUAL_HEX8(0xDD, out[n - 2]);
}

void test_kiss_roundtrip() {
  uint8_t out[128];
  const char* path[] = {"WIDE1-1", "WIDE2-1"};
  size_t n = kissEncodeUiFrame("PY2XX-9", "APRS", path, 2, "!4903.50N/07201.75W-Test", out, sizeof out);
  TEST_ASSERT_TRUE(n > 0);
  KissRx st; bool got = false;
  for (size_t i = 0; i < n; i++) got = kissDeframe(out[i], st);
  TEST_ASSERT_TRUE(got);
  char tnc[200];
  TEST_ASSERT_TRUE(ax25ToTnc2(st.buf, st.len, tnc, sizeof tnc));
  TEST_ASSERT_EQUAL_STRING("PY2XX-9>APRS,WIDE1-1,WIDE2-1:!4903.50N/07201.75W-Test", tnc);
  TEST_ASSERT_FALSE(ax25ToTnc2(st.buf, st.len, tnc, 10));
}

void test_digi_hbit() {
  uint8_t out[128];
  const char* path[] = {"WIDE1-1"};
  size_t n = kissEncodeUiFrame("PY2XX", "APRS", path, 1, "x", out, sizeof out);
  KissRx st;
  for (size_t i = 0; i < n; i++) kissDeframe(out[i], st);
  st.buf[20] |= 0x80;  // H bit on the digi SSID byte
  char tnc[64];
  TEST_ASSERT_TRUE(ax25ToTnc2(st.buf, st.len, tnc, sizeof tnc));
  TEST_ASSERT_EQUAL_STRING("PY2XX>APRS,WIDE1-1*:x", tnc);
}

void test_format_position() {
  char b[64];
  TEST_ASSERT_TRUE(formatAprsPosition(49.058333, -72.029167, '/', '-', "Test", b, sizeof b));
  TEST_ASSERT_EQUAL_STRING("!4903.50N/07201.75W-Test", b);
  TEST_ASSERT_TRUE(formatAprsPosition(10.999999, 0.0, '/', '-', "", b, sizeof b));
  TEST_ASSERT_EQUAL_STRING("!1100.00N/00000.00E-", b);
  TEST_ASSERT_FALSE(formatAprsPosition(1, 1, '/', '-', "Test", b, 10));
}

void test_tx_call_invalid() {
  TEST_ASSERT_FALSE(aprsTxCallValid("N0CALL"));
  TEST_ASSERT_FALSE(aprsTxCallValid("NOCALL-5"));
  TEST_ASSERT_FALSE(aprsTxCallValid(""));
  TEST_ASSERT_TRUE(aprsTxCallValid("py2xx-9"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_kiss_escape); RUN_TEST(test_kiss_roundtrip); RUN_TEST(test_digi_hbit);
  RUN_TEST(test_format_position); RUN_TEST(test_tx_call_invalid);
  return UNITY_END();
}
