#include <unity.h>
#include "fifo_queue.h"
void setUp() {} void tearDown() {}
void test_fifo_order() {
  FifoQueue<int, 4> q; int v;
  TEST_ASSERT_TRUE(q.empty()); TEST_ASSERT_FALSE(q.pop(v));
  q.push(1); q.push(2); q.push(3);
  TEST_ASSERT_EQUAL(3, q.size());
  TEST_ASSERT_TRUE(q.pop(v)); TEST_ASSERT_EQUAL(1, v);
  TEST_ASSERT_TRUE(q.pop(v)); TEST_ASSERT_EQUAL(2, v);
  TEST_ASSERT_TRUE(q.pop(v)); TEST_ASSERT_EQUAL(3, v);
  TEST_ASSERT_FALSE(q.pop(v));
}
void test_full_drops_oldest() {
  FifoQueue<int, 3> q; int v;
  TEST_ASSERT_TRUE(q.push(1)); TEST_ASSERT_TRUE(q.push(2)); TEST_ASSERT_TRUE(q.push(3));
  TEST_ASSERT_FALSE(q.push(4));  // false = an older entry was dropped
  TEST_ASSERT_EQUAL(3, q.size());
  q.pop(v); TEST_ASSERT_EQUAL(2, v); q.pop(v); TEST_ASSERT_EQUAL(3, v); q.pop(v); TEST_ASSERT_EQUAL(4, v);
}
void test_wraparound() {
  FifoQueue<int, 2> q; int v;
  for (int i = 0; i < 20; ++i) { q.push(i); TEST_ASSERT_TRUE(q.pop(v)); TEST_ASSERT_EQUAL(i, v); }
  TEST_ASSERT_TRUE(q.empty());
}
int main() { UNITY_BEGIN(); RUN_TEST(test_fifo_order); RUN_TEST(test_full_drops_oldest); RUN_TEST(test_wraparound); return UNITY_END(); }
