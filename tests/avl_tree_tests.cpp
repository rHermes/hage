#include <doctest/doctest.h>

#include <hage/ds/avl_tree.hpp>

using namespace hage;

TEST_SUITE_BEGIN("data_structures");

TEST_CASE("Simple avl tests")
{
  ds::AVLTree<int, int> tree;

  auto res1 = tree.try_emplace(10, 23);
  REQUIRE_UNARY(res1.second);

  auto res2 = tree.try_emplace(100, 10);
  REQUIRE_UNARY(res2.second);

  SUBCASE("try_emplace should return iterator and false for existing key")
  {
    auto res3 = tree.try_emplace(10, 0xf001);
    REQUIRE_UNARY_FALSE(res3.second);
    REQUIRE_EQ(res1.first, res3.first);
  }

  REQUIRE_EQ(tree.size(), 2);

  REQUIRE_UNARY(tree.contains(100));
  REQUIRE_UNARY_FALSE(tree.contains(200));

  SUBCASE("We should be able to use ++ to iterate")
  {
    auto it = tree.find(10);
    REQUIRE_EQ(it->key(), 10);
    REQUIRE_EQ(it->value(), 23);

    it++;

    REQUIRE_EQ(it->key(), 100);
    REQUIRE_EQ(it->value(), 10);

    ++it;

    REQUIRE_EQ(it, tree.end());
  }

  SUBCASE("We should be able to use -- to iteratte")
  {
    auto it = tree.find(10);
    REQUIRE_EQ(it->key(), 10);
    REQUIRE_EQ(it->value(), 23);

    REQUIRE_EQ((it++)->key(), 10);
    REQUIRE_EQ(it->key(), 100);
    REQUIRE_EQ(it->value(), 10);
  }
}

TEST_CASE("AVL Tree iterator tests")
{
  ds::AVLTree<int, int> tree;

  SUBCASE("Forward iterator has to be a bidirectional iterator")
  {
    REQUIRE_UNARY(std::bidirectional_iterator<decltype(tree)::iterator>);
  }

  SUBCASE("Reverse iterator has to be a bidirectional iterator")
  {
    REQUIRE_UNARY(std::bidirectional_iterator<decltype(tree)::reverse_iterator>);
  }

  constexpr int N = 10;
  // We have to insert a 100 elements.
  for (int i = 0; i < N; i++) {
    auto [it, inserted] = tree.try_emplace(i, N - i);
    REQUIRE_UNARY(inserted);

    auto itEnd = tree.end();
    auto itPrev = std::prev(itEnd);
    REQUIRE_EQ(it, itPrev);
  }

  SUBCASE("Forward iteration should work")
  {
    auto it = tree.begin();
    int i = 0;
    while (it != tree.end()) {
      REQUIRE_EQ(it->key(), i);
      REQUIRE_EQ(it->value(), N - i);
      it++;
      i++;
    }
  }

  SUBCASE("Reverse iteration should work")
  {
    auto it = tree.rbegin();
    int i = N - 1;
    while (it != tree.rend()) {
      REQUIRE_EQ(it->key(), i);
      REQUIRE_EQ(it->value(), N - i);
      it++;
      i--;
    }
  }
}

TEST_SUITE_END();
