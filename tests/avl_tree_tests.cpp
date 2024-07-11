#include <doctest/doctest.h>

#include <hage/ds/avl_tree.hpp>

#include <algorithm>
#include <iostream>
#include <map>
#include <numeric>
#include <random>

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

  SUBCASE("We should be able to use -- to iterate")
  {
    auto it = tree.find(10);
    REQUIRE_EQ(it->key(), 10);
    REQUIRE_EQ(it->value(), 23);

    REQUIRE_EQ((it++)->key(), 10);
    REQUIRE_EQ(it->key(), 100);
    REQUIRE_EQ(it->value(), 10);
  }

  SUBCASE("Erase should work")
  {
    auto it = tree.find(10);
    REQUIRE_NE(it, tree.end());
    REQUIRE_UNARY(tree.contains(10));

    auto it2 = tree.erase(it);
    REQUIRE_NE(it2, tree.end());
    REQUIRE_EQ(it2->key(), 100);
    REQUIRE_UNARY_FALSE(tree.contains(10));
  }

  SUBCASE("Clearing should return all nodes to zero")
  {
    tree.clear();
    REQUIRE_EQ(tree.size(), 0);

    // inserting should work again
    auto res = tree.try_emplace(10, 0xf001);
    REQUIRE_UNARY(res.second);
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

  constexpr int N = 100;
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

  SUBCASE("Const Forward iteration should work")
  {
    auto it = tree.cbegin();
    int i = 0;
    while (it != tree.cend()) {
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

  SUBCASE("Const Reverse iteration should work")
  {
    auto it = tree.crbegin();
    int i = N - 1;
    while (it != tree.crend()) {
      REQUIRE_EQ(it->key(), i);
      REQUIRE_EQ(it->value(), N - i);
      it++;
      i--;
    }
  }
}

TEST_CASE("AVL erase tests")
{
  ds::AVLTree<int, int> tree;
  std::map<int, int> mirror;

  constexpr int N = 20;
  // We have to insert a 100 elements.
  for (int i = 0; i < N; i++) {
    auto [it, inserted] = tree.try_emplace(i, N - i);
    REQUIRE_UNARY(inserted);

    mirror.emplace(i, N - i);

    auto itEnd = tree.end();
    auto itPrev = std::prev(itEnd);
    REQUIRE_EQ(it, itPrev);
  }

  // Ok, here we go
  std::vector<int> toDelete(N);
  std::iota(toDelete.begin(), toDelete.end(), 0);

  SUBCASE("Erasing randomly should work")
  {
    std::ranlux48 rng{ 24 };
    std::ranges::shuffle(toDelete, rng);
    for (auto toDel : toDelete) {
      INFO("Doing: ", toDel);

      std::cout << "Going to delete: " << toDel << "\n";
      std::cout << tree.print_dot_tree();

      auto it1 = tree.find(toDel);
      REQUIRE_NE(it1, tree.end());
      auto it2 = mirror.find(toDel);

      REQUIRE_EQ(it1->key(), it2->first);
      REQUIRE_EQ(it1->value(), it2->second);

      auto it3 = tree.erase(it1);
      auto it4 = mirror.erase(it2);

      if (it3 == tree.end()) {
        REQUIRE_EQ(it4, mirror.end());
      } else {
        REQUIRE_EQ(it3->key(), it4->first);
        REQUIRE_EQ(it3->value(), it4->second);
      }
    }
  }
}

TEST_SUITE_END();
