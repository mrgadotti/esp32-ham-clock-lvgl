#include <unity.h>
#include <string.h>
#include "aprs_message.h"
void setUp() {} void tearDown() {}

void test_msg_for_me() {
  AprsMessage m;
  TEST_ASSERT_TRUE(aprsParseMessage("PU2ABC-9>APRS,TCPIP*::PY2XX-7  :Hello there{123", "PY2XX-7", m));
  TEST_ASSERT_EQUAL_STRING("PU2ABC-9", m.from);
  TEST_ASSERT_EQUAL_STRING("Hello there", m.text);
  TEST_ASSERT_EQUAL_STRING("123", m.msgNo);
  TEST_ASSERT_EQUAL_CHAR(' ', m.sym[0]); TEST_ASSERT_EQUAL_CHAR(' ', m.sym[1]);
  TEST_ASSERT_EQUAL(-1, m.distKm);
  TEST_ASSERT_FALSE(m.hasLatLon); TEST_ASSERT_EQUAL(0, m.utc);
}
void test_msg_no_msgno() {
  AprsMessage m;
  TEST_ASSERT_TRUE(aprsParseMessage("A1B>APRS::PY2XX    :hi", "PY2XX", m));
  TEST_ASSERT_EQUAL_STRING("", m.msgNo);
  TEST_ASSERT_EQUAL_STRING("hi", m.text);
}
void test_msg_case_insensitive() {
  AprsMessage m;
  TEST_ASSERT_TRUE(aprsParseMessage("a1b>aprs::py2xx    :hi", "PY2XX", m));
  TEST_ASSERT_EQUAL_STRING("A1B", m.from);
}
void test_msg_not_for_me() {
  AprsMessage m;
  TEST_ASSERT_FALSE(aprsParseMessage("A1B>APRS::PY2XX-7  :hi", "PY2XX", m));
  TEST_ASSERT_FALSE(aprsParseMessage("A1B>APRS::PY2XX    :hi", "PY2XX-7", m));
  TEST_ASSERT_FALSE(aprsParseMessage("A1B>APRS::N0CALL   :hi", "PY2XX", m));
  TEST_ASSERT_FALSE(aprsParseMessage("A1B>APRS::BLN1     :text", "PY2XX", m));
}
void test_msg_ack_rej_ignored() {
  AprsMessage m;
  TEST_ASSERT_FALSE(aprsParseMessage("A1B>APRS::PY2XX    :ack123", "PY2XX", m));
  TEST_ASSERT_FALSE(aprsParseMessage("A1B>APRS::PY2XX    :rej7", "PY2XX", m));
  TEST_ASSERT_TRUE(aprsParseMessage("A1B>APRS::PY2XX    :acknowledged ok", "PY2XX", m));
  TEST_ASSERT_EQUAL_STRING("acknowledged ok", m.text);
}
void test_msg_empty_text() {
  AprsMessage m;
  TEST_ASSERT_FALSE(aprsParseMessage("A1B>APRS::PY2XX    :", "PY2XX", m));
  TEST_ASSERT_FALSE(aprsParseMessage("A1B>APRS::PY2XX    :{12", "PY2XX", m));
}
void test_msg_not_a_message() {
  AprsMessage m;
  TEST_ASSERT_FALSE(aprsParseMessage("A1B>APRS:!4903.50N/07201.75W-x", "PY2XX", m));
  TEST_ASSERT_FALSE(aprsParseMessage(nullptr, "PY2XX", m));
  TEST_ASSERT_FALSE(aprsParseMessage("A1B>APRS::PY2XX    :hi", nullptr, m));
}
void test_msg_text_sanitized_truncated() {
  AprsMessage m;
  char line[200] = "A1B>APRS::PY2XX    :";
  size_t n = strlen(line);
  for (int i = 0; i < 80; i++) line[n + i] = 'a' + i % 26;
  line[n + 80] = 0;
  TEST_ASSERT_TRUE(aprsParseMessage(line, "PY2XX", m));
  TEST_ASSERT_EQUAL(67, strlen(m.text));
  TEST_ASSERT_TRUE(aprsParseMessage("A1B>APRS::PY2XX    :a\r\nb\xC3\xA9" "c", "PY2XX", m));
  TEST_ASSERT_EQUAL_STRING("a  bc", m.text);
}
void test_msg_same() {
  AprsMessage a, b;
  TEST_ASSERT_TRUE(aprsParseMessage("A1B>APRS::PY2XX    :hi{1", "PY2XX", a));
  TEST_ASSERT_TRUE(aprsParseMessage("A1B>APRS::PY2XX    :hi{1", "PY2XX", b));
  TEST_ASSERT_TRUE(aprsMsgSame(a, b));
  TEST_ASSERT_TRUE(aprsParseMessage("A1B>APRS::PY2XX    :hi{2", "PY2XX", b));
  TEST_ASSERT_FALSE(aprsMsgSame(a, b));
  TEST_ASSERT_TRUE(aprsParseMessage("A1B>APRS::PY2XX    :ho{1", "PY2XX", b));
  TEST_ASSERT_FALSE(aprsMsgSame(a, b));
  TEST_ASSERT_TRUE(aprsParseMessage("A1C>APRS::PY2XX    :hi{1", "PY2XX", b));
  TEST_ASSERT_FALSE(aprsMsgSame(a, b));
}
void test_ack_info_padding() {
  char o[32];
  TEST_ASSERT_TRUE(aprsFormatAckInfo("PU1ABC", "123", o, sizeof o));
  TEST_ASSERT_EQUAL_STRING(":PU1ABC   :ack123", o);
}
void test_ack_info_9char_call_5char_msgno() {
  char o[32];
  TEST_ASSERT_TRUE(aprsFormatAckInfo("PU1ABC-15", "AB123", o, sizeof o));
  TEST_ASSERT_EQUAL_STRING(":PU1ABC-15:ackAB123", o);
}
void test_ack_info_rejects() {
  char o[32] = "keep";
  TEST_ASSERT_FALSE(aprsFormatAckInfo("PU1ABC", "", o, sizeof o));
  TEST_ASSERT_FALSE(aprsFormatAckInfo("", "1", o, sizeof o));
  TEST_ASSERT_FALSE(aprsFormatAckInfo("PU1ABC-123", "1", o, sizeof o));
  TEST_ASSERT_EQUAL_STRING("keep", o);
}
void test_ack_info_cap_too_small() {
  char o[18] = "keep";
  TEST_ASSERT_FALSE(aprsFormatAckInfo("PU1ABC", "123", o, 17));  // needs 18 with NUL
  TEST_ASSERT_EQUAL_STRING("keep", o);
  TEST_ASSERT_TRUE(aprsFormatAckInfo("PU1ABC", "123", o, 18));
}

int main() { UNITY_BEGIN(); RUN_TEST(test_msg_for_me); RUN_TEST(test_msg_no_msgno);
  RUN_TEST(test_msg_case_insensitive); RUN_TEST(test_msg_not_for_me); RUN_TEST(test_msg_ack_rej_ignored);
  RUN_TEST(test_msg_empty_text); RUN_TEST(test_msg_not_a_message); RUN_TEST(test_msg_text_sanitized_truncated);
  RUN_TEST(test_msg_same);
  RUN_TEST(test_ack_info_padding); RUN_TEST(test_ack_info_9char_call_5char_msgno);
  RUN_TEST(test_ack_info_rejects); RUN_TEST(test_ack_info_cap_too_small); return UNITY_END(); }
