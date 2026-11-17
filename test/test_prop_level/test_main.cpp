#include <unity.h>
#include "prop_level.h"
void setUp() {} void tearDown() {}
void test_cond() {
  TEST_ASSERT_EQUAL(PropCond::Good, propCondOf("Good")); TEST_ASSERT_EQUAL(PropCond::Good, propCondOf("good"));
  TEST_ASSERT_EQUAL(PropCond::Fair, propCondOf("FAIR")); TEST_ASSERT_EQUAL(PropCond::Poor, propCondOf("Poor"));
  TEST_ASSERT_EQUAL(PropCond::Poor, propCondOf("Poorish"));  // substring match, as the original
  TEST_ASSERT_EQUAL(PropCond::Unknown, propCondOf("")); TEST_ASSERT_EQUAL(PropCond::Unknown, propCondOf("Band Closed"));
  TEST_ASSERT_EQUAL(PropCond::Unknown, propCondOf("xxxxxxxxGood")); }  // only the first 7 chars are examined
void test_kindex() {
  TEST_ASSERT_EQUAL(KLevel::Low, kIndexLevel("0")); TEST_ASSERT_EQUAL(KLevel::Low, kIndexLevel("3"));
  TEST_ASSERT_EQUAL(KLevel::Mid, kIndexLevel("4")); TEST_ASSERT_EQUAL(KLevel::Mid, kIndexLevel("5"));
  TEST_ASSERT_EQUAL(KLevel::High, kIndexLevel("6")); TEST_ASSERT_EQUAL(KLevel::High, kIndexLevel("9"));
  TEST_ASSERT_EQUAL(KLevel::None, kIndexLevel("")); TEST_ASSERT_EQUAL(KLevel::None, kIndexLevel("--")); TEST_ASSERT_EQUAL(KLevel::None, kIndexLevel("n/a")); }
int main() { UNITY_BEGIN(); RUN_TEST(test_cond); RUN_TEST(test_kindex); return UNITY_END(); }
