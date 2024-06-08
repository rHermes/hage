#include <doctest/doctest.h>

#include "fast_pimpl_test.hpp"

using namespace hage;

TEST_SUITE_BEGIN("forward_declared_storage");

TEST_CASE("Does move work")
{
  test::FastPimplTest hey;

  hey.setB("pirate!");
  REQUIRE_EQ(hey.getB(), "pirate!");

  hey.setA(10);

  SUBCASE("Copy assignment")
  {
    test::FastPimplTest hello;

    hello = hey;
    REQUIRE_EQ(hey.getB(), "pirate!");
    REQUIRE_EQ(hello.getB(), "pirate!");
  }

  SUBCASE("Move constructor")
  {
    test::FastPimplTest hello = std::move(hey);
    REQUIRE_UNARY(hey.getB().empty());
    REQUIRE_EQ(hello.getB(), "pirate!");
  }

  SUBCASE("Move assignment")
  {
    test::FastPimplTest hello;
    hello = std::move(hey);
    REQUIRE_UNARY(hey.getB().empty());
    REQUIRE_EQ(hello.getB(), "pirate!");
  }
}

TEST_SUITE_END();