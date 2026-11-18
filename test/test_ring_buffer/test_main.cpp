#include <unity.h>
#include <initializer_list>
#include "ring_buffer.h"
#include "types.h"
void setUp() {} void tearDown() {}
static void expect(RingBuffer<int, 4>& r, std::initializer_list<int> want) {  // newest first
  TEST_ASSERT_EQUAL(want.size(), r.size()); size_t i = 0; for (int w : want) TEST_ASSERT_EQUAL(w, r.at(i++)); }
void test_overflow() { RingBuffer<int, 2> r; r.pushFront(1); r.pushFront(2); r.pushFront(3);
  TEST_ASSERT_EQUAL(2, r.size()); TEST_ASSERT_EQUAL(3, r.at(0)); TEST_ASSERT_EQUAL(2, r.at(1)); r.clear(); TEST_ASSERT_EQUAL(0, r.size()); }
void test_fill_and_wrap_many_times() {
  RingBuffer<int, 4> r; expect(r, {});
  r.pushFront(1); expect(r, {1}); r.pushFront(2); expect(r, {2, 1}); r.pushFront(3); r.pushFront(4); expect(r, {4, 3, 2, 1});
  for (int v = 5; v < 40; ++v) { r.pushFront(v); expect(r, {v, v - 1, v - 2, v - 3}); } }
void test_capacity_one() { RingBuffer<int, 1> r; r.pushFront(7); r.pushFront(8); TEST_ASSERT_EQUAL(1, r.size()); TEST_ASSERT_EQUAL(8, r.at(0));
  TEST_ASSERT_TRUE(r.moveToFrontIf([](const int& v) { return v == 8; }, 9)); TEST_ASSERT_EQUAL(9, r.at(0)); }
void test_clear_then_reuse() { RingBuffer<int, 4> r; for (int v = 1; v <= 6; ++v) r.pushFront(v); r.clear(); expect(r, {});
  r.pushFront(10); r.pushFront(11); expect(r, {11, 10}); }
void test_move_to_front() { RingBuffer<int, 4> r; r.pushFront(1); r.pushFront(2); r.pushFront(3);
  TEST_ASSERT_TRUE(r.moveToFrontIf([](const int& v) { return v == 1; }, 10));
  TEST_ASSERT_EQUAL(3, r.size()); TEST_ASSERT_EQUAL(10, r.at(0)); TEST_ASSERT_EQUAL(3, r.at(1)); TEST_ASSERT_EQUAL(2, r.at(2));
  TEST_ASSERT_FALSE(r.moveToFrontIf([](const int& v) { return v == 99; }, 5)); TEST_ASSERT_EQUAL(3, r.size()); }
// Full and wrapped so head_ != 0: head / middle / tail matches.
static RingBuffer<int, 4> wrapped() { RingBuffer<int, 4> r; for (int v = 1; v <= 6; ++v) r.pushFront(v); return r; }  // {6,5,4,3}
void test_move_head() { auto r = wrapped(); TEST_ASSERT_TRUE(r.moveToFrontIf([](const int& v) { return v == 6; }, 60)); expect(r, {60, 5, 4, 3}); }
void test_move_middle() { auto r = wrapped(); TEST_ASSERT_TRUE(r.moveToFrontIf([](const int& v) { return v == 4; }, 40)); expect(r, {40, 6, 5, 3}); }
void test_move_tail() { auto r = wrapped(); TEST_ASSERT_TRUE(r.moveToFrontIf([](const int& v) { return v == 3; }, 30)); expect(r, {30, 6, 5, 4});
  r.pushFront(7); expect(r, {7, 30, 6, 5}); }
void test_move_first_match_only() { RingBuffer<int, 4> r; r.pushFront(1); r.pushFront(2); r.pushFront(1);  // {1,2,1}
  TEST_ASSERT_TRUE(r.moveToFrontIf([](const int& v) { return v == 1; }, 9)); expect(r, {9, 2, 1}); }
// Non-power-of-two sizes: push 2N+1 so head_ wraps twice, then check order and moveToFrontIf at head/middle/tail.
template <size_t N> static void wrapCheck() {
  RingBuffer<int, N> r; const int total = 2 * (int)N + 1;
  for (int v = 1; v <= total; ++v) r.pushFront(v);
  TEST_ASSERT_EQUAL(N, r.size());
  for (size_t i = 0; i < N; ++i) TEST_ASSERT_EQUAL(total - (int)i, r.at(i));
  const size_t pos[3] = {0, N / 2, N - 1};  // head, middle, tail
  for (size_t p : pos) {
    RingBuffer<int, N> c = r; int target = total - (int)p;
    TEST_ASSERT_TRUE(c.moveToFrontIf([&](const int& v) { return v == target; }, -1));
    TEST_ASSERT_EQUAL(N, c.size()); TEST_ASSERT_EQUAL(-1, c.at(0));
    size_t k = 1;  // the rest keep their relative order, newest first
    for (size_t i = 0; i < N; ++i) if (i != p) TEST_ASSERT_EQUAL(total - (int)i, c.at(k++));
    c.pushFront(99); TEST_ASSERT_EQUAL(99, c.at(0)); TEST_ASSERT_EQUAL(-1, c.at(1));
  }
}
void test_wrap_n3() { wrapCheck<3>(); }
void test_wrap_n30() { wrapCheck<30>(); }
void test_types_pod() { static_assert(__is_trivial(Spot) && __is_trivial(PropagationData), "POD");
  RingBuffer<Spot, 3> r; Spot s = {}; s.khz = 7074; r.pushFront(s); TEST_ASSERT_EQUAL_UINT32(7074, r.at(0).khz); }
int main() { UNITY_BEGIN(); RUN_TEST(test_overflow); RUN_TEST(test_fill_and_wrap_many_times); RUN_TEST(test_capacity_one); RUN_TEST(test_clear_then_reuse);
  RUN_TEST(test_move_to_front); RUN_TEST(test_move_head); RUN_TEST(test_move_middle); RUN_TEST(test_move_tail); RUN_TEST(test_move_first_match_only);
  RUN_TEST(test_wrap_n3); RUN_TEST(test_wrap_n30); RUN_TEST(test_types_pod); return UNITY_END(); }
