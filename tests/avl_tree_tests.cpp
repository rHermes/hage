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

TEST_CASE("AVL erasure specific tests")
{
  // These are handcrafted trees to test for common mistake I've found during debugging.
  ds::AVLTree<int, int> tree;
  std::map<int, int> mirror;

  const auto emplace = [&](const int key, const int val) {
    auto [it1, inserted1] = tree.try_emplace(key, val);
    auto [it2, inserted2] = mirror.try_emplace(key, val);
    REQUIRE_EQ(inserted1, inserted2);

    if (inserted1) {
      REQUIRE_EQ(it1->key(), it2->first);
      REQUIRE_EQ(it1->value(), it2->second);
    }

    return std::make_pair(it1, inserted1);
  };

  const auto erase = [&](const int key) {
    auto findIt1 = tree.find(key);
    auto findIt2 = mirror.find(key);
    auto it1 = tree.erase(findIt1);
    auto it2 = mirror.erase(findIt2);

    if (it2 == mirror.end()) {
      REQUIRE_UNARY(it1 == tree.end());
    } else {
      REQUIRE_UNARY_FALSE(it1 == tree.end());
      REQUIRE_EQ(it1->key(), it2->first);
      REQUIRE_EQ(it1->value(), it2->second);
    }

    return it1;
  };

  SUBCASE("Root with only left leaf")
  {
    emplace(10, 1);
    emplace(7, 2);

    SUBCASE("Deleting root")
    {
      auto it = erase(10);
      REQUIRE_EQ(it, tree.end());
    }

    SUBCASE("Deleting leaf")
    {
      auto it = erase(7);
      REQUIRE_EQ(it->key(), 10);
    }

    SUBCASE("Root then leaf")
    {
      auto it = erase(10);
      REQUIRE_EQ(it, tree.end());

      auto it2 = erase(7);
      REQUIRE_EQ(it2, tree.end());
    }
  }

  SUBCASE("Delete root with next has right child but not left")
  {
    emplace(15, 1);
    emplace(10, 2);
    emplace(32, 3);
    emplace(11, 4);
    emplace(24, 5);
    emplace(39, 6);
    emplace(34, 12);
    emplace(7, 7);
    emplace(13, 8);
    emplace(19, 9);
    emplace(25, 10);
    emplace(21, 11);

    erase(15);

    auto it = tree.find(21);
    REQUIRE_NE(it, tree.end());
  }

  SUBCASE("Delete direct right child, with right child")
  {
    emplace(15, 1);
    emplace(10, 2);
    emplace(32, 3);
    emplace(16, 4);
    emplace(35, 5);
    emplace(11, 6);
    emplace(39, 7);

    erase(32);

    auto it = tree.find(39);
    REQUIRE_NE(it, tree.end());
  }
}

TEST_CASE("AVL erasure grind tests")
{
  ds::AVLTree<int, int> tree;
  std::map<int, int> mirror;

  constexpr int N = 100000;
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
    std::ranlux48 rng{ 123 };
    std::ranges::shuffle(toDelete, rng);
    for (auto toDel : toDelete) {
      INFO("Doing: ", toDel);

      /*
      std::cout << "Going to delete: " << toDel << "\n";
      std::cout << tree.print_dot_tree();
       */

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
