#include <doctest/doctest.h>

#if __has_include(<charconv>)

#include <hage/core/charconv.hpp>
#include <string>

TEST_SUITE_BEGIN("core");

TEST_CASE("Charconv integer overloads")
{
  const std::string lel = "123";
  constexpr auto lelInt = 123;

  SUBCASE("Standard overload should be enabled")
  {
    int out;
    hage::from_chars(lel.data(), lel.data() + lel.size(), out);
    REQUIRE_EQ(out, lelInt);
  }

  SUBCASE("String should work fine")
  {
    int out;
    hage::from_chars(lel, out);
    REQUIRE_EQ(out, lelInt);
  }
}

#if defined __cpp_lib_to_chars

TEST_CASE("Charconv floats overloads")
{
  const std::string lel = "1.3";
  constexpr auto lelInt = 1.3;

  SUBCASE("Standard overload should be enabled")
  {
    double out;
    hage::from_chars(lel.data(), lel.data() + lel.size(), out);
    REQUIRE_EQ(out, lelInt);
  }

  SUBCASE("String should work fine")
  {
    double out;
    hage::from_chars(lel, out);
    REQUIRE_EQ(out, lelInt);
  }
}
#endif
TEST_SUITE_END();

#endif