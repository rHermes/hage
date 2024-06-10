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
}

TEST_SUITE_END();
