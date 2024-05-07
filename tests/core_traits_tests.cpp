#include <doctest/doctest.h>

#include <hage/core/traits.hpp>

TEST_SUITE_BEGIN("core");

TEST_CASE("all_same")
{
  REQUIRE_UNARY(hage::all_same_v<int, int, int>);
  REQUIRE_UNARY(!hage::all_same_v<double, int, double>);
  REQUIRE_UNARY(hage::all_same_v<int>);
  REQUIRE_UNARY(hage::all_same_v<>);
  REQUIRE_UNARY(hage::all_same_v<int, int>);
}

TEST_SUITE_END();