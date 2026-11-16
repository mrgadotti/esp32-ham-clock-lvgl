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

void test_hardening() {
  char b[64];
  TEST_ASSERT_TRUE(formatAprsPosition(0, -72.9999999, '/', '-', "", b, sizeof b));
  TEST_ASSERT_EQUAL_STRING("!0000.00N/07300.00W-", b);
  strcpy(b, "keep");
  TEST_ASSERT_FALSE(formatAprsPosition(91, 0, '/', '-', "", b, sizeof b));
  TEST_ASSERT_FALSE(formatAprsPosition(__builtin_nan(""), 0, '/', '-', "", b, sizeof b));
  TEST_ASSERT_FALSE(formatAprsPosition(0, 181, '/', '-', "", b, sizeof b));
  TEST_ASSERT_FALSE(formatAprsPosition(1, 1, '/', '-', "Test", b, 10));
  TEST_ASSERT_EQUAL_STRING("keep", b);

  uint8_t out[64]; KissRx st; bool got = false;
  size_t n = kissEncodeUiFrame("PY2XX", "APRS", nullptr, 0, "hello", out, sizeof out);
  for (size_t i = 0; i < n; i++) got = kissDeframe(out[i], st);
  TEST_ASSERT_TRUE(got);
  char t[64]; strcpy(t, "keep");
  TEST_ASSERT_FALSE(ax25ToTnc2(st.buf, 15, t, sizeof t));    // too short
  TEST_ASSERT_FALSE(ax25ToTnc2(st.buf, st.len, t, 12));      // buffer too small
  uint8_t f[20]; memcpy(f, st.buf, 16); f[13] &= 0xFE;       // no end-of-address bit
  TEST_ASSERT_FALSE(ax25ToTnc2(f, 16, t, sizeof t));
  TEST_ASSERT_EQUAL_STRING("keep", t);

  // overflow: dropped, next frame still decodes
  KissRx s2; kissDeframe(0xC0, s2); kissDeframe(0x00, s2);
  for (int i = 0; i < 700; i++) TEST_ASSERT_FALSE(kissDeframe('A', s2));
  TEST_ASSERT_FALSE(kissDeframe(0xC0, s2));
  got = false;
  for (size_t i = 0; i < n; i++) got = kissDeframe(out[i], s2);
  TEST_ASSERT_TRUE(got);
  TEST_ASSERT_TRUE(ax25ToTnc2(s2.buf, s2.len, t, sizeof t));

  // truncated escape: FESC then FEND is not a frame
  KissRx s3; kissDeframe(0xC0, s3); kissDeframe(0x00, s3); kissDeframe('A', s3); kissDeframe(0xDB, s3);
  TEST_ASSERT_FALSE(kissDeframe(0xC0, s3));

  // non-data command ignored
  KissRx s4; kissDeframe(0xC0, s4); kissDeframe(0x01, s4); kissDeframe('A', s4);
  TEST_ASSERT_FALSE(kissDeframe(0xC0, s4));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_kiss_escape); RUN_TEST(test_kiss_roundtrip); RUN_TEST(test_digi_hbit);
  RUN_TEST(test_format_position); RUN_TEST(test_tx_call_invalid); RUN_TEST(test_hardening);
  return UNITY_END();
}
