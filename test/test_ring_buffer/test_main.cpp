#include <unity.h>
#include "ring_buffer.h"
#include "types.h"
void setUp() {} void tearDown() {}
void test_overflow() { RingBuffer<int, 2> r; r.pushFront(1); r.pushFront(2); r.pushFront(3);
  TEST_ASSERT_EQUAL(2, r.size()); TEST_ASSERT_EQUAL(3, r.at(0)); TEST_ASSERT_EQUAL(2, r.at(1)); r.clear(); TEST_ASSERT_EQUAL(0, r.size()); }
void test_move_to_front() { RingBuffer<int, 4> r; r.pushFront(1); r.pushFront(2); r.pushFront(3);
  TEST_ASSERT_TRUE(r.moveToFrontIf([](const int& v) { return v == 1; }, 10));
  TEST_ASSERT_EQUAL(3, r.size()); TEST_ASSERT_EQUAL(10, r.at(0)); TEST_ASSERT_EQUAL(3, r.at(1)); TEST_ASSERT_EQUAL(2, r.at(2));
  TEST_ASSERT_FALSE(r.moveToFrontIf([](const int& v) { return v == 99; }, 5)); TEST_ASSERT_EQUAL(3, r.size()); }
void test_types_pod() { static_assert(__is_trivial(Spot) && __is_trivial(GpsState) && __is_trivial(PropagationData), "POD");
  RingBuffer<Spot, 3> r; Spot s = {}; s.khz = 7074; r.pushFront(s); TEST_ASSERT_EQUAL_UINT32(7074, r.at(0).khz); }
int main() { UNITY_BEGIN(); RUN_TEST(test_overflow); RUN_TEST(test_move_to_front); RUN_TEST(test_types_pod); return UNITY_END(); }
