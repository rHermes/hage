#include <doctest/doctest.h>

#include <hage/atomic/atomic.hpp>

TEST_SUITE_BEGIN("atomic");

TEST_CASE_TEMPLATE("Simple atomic tests", atomic_type, hage::atomic<std::int64_t>)
{
  atomic_type wow;

  wow.store(3);
  REQUIRE_EQ(3, wow.load());
}

// TODO(rHermes): add overloads for floats
TEST_CASE_TEMPLATE("Waiting tests", atomic_type, hage::atomic<std::int64_t>, hage::atomic<float>)
{
  using namespace std::chrono_literals;

  atomic_type wow;

  wow.store(0);

  // WE should wait
  REQUIRE_UNARY_FALSE(wow.wait_for(0, 10ms));
}

TEST_SUITE_END();