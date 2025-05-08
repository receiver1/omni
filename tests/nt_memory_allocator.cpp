#include <deque>
#include "gtest/gtest.h"
#include "shadowsyscall.hpp"

using shadow::huge_allocator;
using shadow::rw_allocator;
using shadow::rwx_allocator;
using shadow::rx_allocator;

template <typename Allocator>
void vector_roundtrip() {
  std::vector<int, Allocator> v;
  v.reserve(128);
  for (int i = 0; i < 128; ++i)
    v.push_back(i);
  for (int i = 0; i < 128; ++i)
    ASSERT_EQ(v[i], i);
}

template <typename Allocator>
void deque_roundtrip() {
  std::deque<int, Allocator> dq;
  for (int i = 0; i < 64; ++i)
    dq.push_front(i);
  for (int i = 0; i < 64; ++i) {
    ASSERT_EQ(dq.back(), i);
    dq.pop_back();
  }
  ASSERT_TRUE(dq.empty());
}

template <typename Allocator>
void list_roundtrip() {
  std::list<int, Allocator> lst;
  for (int i = 0; i < 32; ++i)
    lst.push_back(i * 2);
  int expected = 0;
  for (int v : lst) {
    ASSERT_EQ(v, expected);
    expected += 2;
  }
  lst.clear();
  ASSERT_TRUE(lst.empty());
}

template <typename Allocator>
void unordered_map_roundtrip() {
  std::unordered_map<int, int, std::hash<int>, std::equal_to<int>, Allocator> mp;
  for (int i = 0; i < 32; ++i)
    mp.emplace(i, i + 1);
  for (int i = 0; i < 32; ++i)
    ASSERT_EQ(mp.at(i), i + 1);
}

template <typename Allocator>
void traits_roundtrip() {
  using traits = std::allocator_traits<Allocator>;
  Allocator alloc;
  auto* p = traits::allocate(alloc, 4);
  for (int i = 0; i < 4; ++i)
    traits::construct(alloc, p + i, i);
  ASSERT_EQ(*(p + 3), 3);
  for (int i = 0; i < 4; ++i)
    traits::destroy(alloc, p + i);
  traits::deallocate(alloc, p, 4);
}

TEST(allocator_rw, vector) {
  vector_roundtrip<rw_allocator<int>>();
}
TEST(allocator_rwx, vector) {
  vector_roundtrip<rwx_allocator<int>>();
}

TEST(allocator_rw, deque) {
  deque_roundtrip<rw_allocator<int>>();
}

TEST(allocator_rw, list) {
  list_roundtrip<rw_allocator<int>>();
}
TEST(allocator_rwx, list) {
  list_roundtrip<rwx_allocator<int>>();
}

TEST(allocator_rw, umap) {
  unordered_map_roundtrip<rw_allocator<std::pair<const int, int>>>();
}

TEST(allocator_rw, traits) {
  traits_roundtrip<rw_allocator<int>>();
}
TEST(allocator_rwx, traits) {
  traits_roundtrip<rwx_allocator<int>>();
}

TEST(allocator_rx, death_vector) {
  auto function = []() {
    std::vector<int, rx_allocator<int>> v;
    v.reserve(8);
    v.push_back(1);
  };
  // Writing to RX memory will cause AV
  EXPECT_DEATH(function(), "");
}

TEST(allocator_rebind, to_double) {
  using alloc_int = rw_allocator<int>;
  using alloc_dbl = typename alloc_int::template rebind<double>::other;
  std::vector<double, alloc_dbl> v;
  v.push_back(3.14);
  ASSERT_DOUBLE_EQ(v.front(), 3.14);
}

TEST(allocator_traits, always_equal) {
  ASSERT_TRUE(std::allocator_traits<rw_allocator<int>>::is_always_equal::value);
}
