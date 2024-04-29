#include <doctest/doctest.h>

#include <filesystem>

#include <hage/logging/rotating_file_sink.hpp>

namespace {

struct FalseSplitter
{
  [[nodiscard]] constexpr bool operator()(const hage::LogFileStats&) { return false; }
};

struct TrueSplitter
{
  [[nodiscard]] constexpr bool operator()(const hage::LogFileStats&) { return true; }
};
}

TEST_SUITE_BEGIN("logging");

TEST_CASE("AndSplitter")
{
  hage::LogFileStats exampleStats1{ .bytes = 10 };

  SUBCASE("Should handle more true")
  {
    hage::AndSplitter splitter{ TrueSplitter{}, TrueSplitter{} };

    REQUIRE_UNARY(splitter(exampleStats1));
  }

  SUBCASE("No splitters should produce true")
  {
    hage::AndSplitter splitter{};

    REQUIRE_UNARY(splitter(exampleStats1));
  }

  SUBCASE("All false should produce false")
  {
    hage::AndSplitter splitter{ FalseSplitter{}, FalseSplitter{} };

    REQUIRE_UNARY_FALSE(splitter(exampleStats1));
  }

  SUBCASE("One false should produce false")
  {
    hage::AndSplitter splitter{ TrueSplitter{}, FalseSplitter{} };

    REQUIRE_UNARY_FALSE(splitter(exampleStats1));
  }
}

TEST_CASE("OrSplitter")
{
  hage::LogFileStats exampleStats1{ .bytes = 10 };

  SUBCASE("All true should be true")
  {
    hage::OrSplitter splitter{ TrueSplitter{}, TrueSplitter{} };

    REQUIRE_UNARY(splitter(exampleStats1));
  }

  SUBCASE("No splitters should produce false")
  {
    hage::OrSplitter splitter;

    REQUIRE_UNARY_FALSE(splitter(exampleStats1));
  }

  SUBCASE("All false should produce false")
  {
    hage::OrSplitter splitter{ FalseSplitter{}, FalseSplitter{} };

    REQUIRE_UNARY_FALSE(splitter(exampleStats1));
  }

  SUBCASE("One false should produce true")
  {
    hage::OrSplitter splitter{ TrueSplitter{}, FalseSplitter{} };

    REQUIRE_UNARY(splitter(exampleStats1));
  }
}

TEST_SUITE_END();