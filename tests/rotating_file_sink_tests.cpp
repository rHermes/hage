#include <doctest/doctest.h>

#include <filesystem>

#include <hage/logging/rotating_file_sink.hpp>

TEST_SUITE_BEGIN("logging");

TEST_CASE("SizeRotater")
{
  hage::SizeRotater rotater{ "test.log", 100 };
  hage::LogFileStats underLimitStat{ .bytes = 10 };
  hage::LogFileStats overLimitStat{ .bytes = 150 };

  REQUIRE_EQ(rotater.getRotateType(), hage::Rotater::Type::MoveBackwards);

  SUBCASE("Should not break on smaller writing")
  {
    CHECK_UNARY_FALSE(rotater.shouldRotate(underLimitStat));
  }

  SUBCASE("Should break bigger files")
  {
    CHECK_UNARY(rotater.shouldRotate(overLimitStat));
  }

  SUBCASE("Zero backlog, should be empty")
  {
    CHECK_UNARY(rotater.generateNames(underLimitStat, 0).empty());
  }

  SUBCASE("One backlog, should return just the base filename")
  {
    const auto names = rotater.generateNames(underLimitStat, 1);
    CHECK_EQ(names.size(), 1);
    CHECK_EQ(names[0], "test.log");
  }

  SUBCASE("More backlog, should return the right patterns")
  {
    const auto names = rotater.generateNames(underLimitStat, 5);
    CHECK_EQ(names.size(), 5);
    CHECK_EQ(names[0], "test.log");
    CHECK_EQ(names[1], "test.log.1");
    CHECK_EQ(names[2], "test.log.2");
    CHECK_EQ(names[3], "test.log.3");
    CHECK_EQ(names[4], "test.log.4");
  }
}

TEST_SUITE_END();