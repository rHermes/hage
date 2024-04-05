//
// Created by rhermes on 4/5/24.
//
#include "test_utils.hpp"
#include <random>

#include <doctest/doctest.h>

std::filesystem::path
hage::test::temp_file_name(const fmt::format_string<std::uint32_t> prefix)
{
  const auto tmpDir = std::filesystem::temp_directory_path();
  std::random_device r;
  std::default_random_engine rnd(r());
  std::uniform_int_distribution<std::uint32_t> dist;

  while (true) {
    const auto pth = tmpDir / fmt::format(prefix, dist(rnd));
    if (!exists(pth)) {
      return pth;
    }
  }
}

TEST_CASE("ScopedTempFile")
{
  SUBCASE("Should remove file if it's created")
  {
    hage::test::ScopedTempFile fs("wow.{}.txt");
  }
}