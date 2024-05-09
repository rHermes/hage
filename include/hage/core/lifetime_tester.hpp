#pragma once

#include <cstdio>
#include <string_view>

namespace hage {
// stolen from jason turner
struct LifetimeTester
{
  explicit LifetimeTester(int) noexcept { std::printf("LifetimeTester::LifetimeTester(int)\n"); }
  LifetimeTester() noexcept { std::printf("LifetimeTester::LifetimeTester()\n"); }
  LifetimeTester(LifetimeTester&&) noexcept { std::printf("LifetimeTester::LifetimeTester(LifetimeTester&&)\n"); }
  LifetimeTester(const LifetimeTester&) noexcept
  {
    std::printf("LifetimeTester::LifetimeTester(const LifetimeTester&)\n");
  }
  ~LifetimeTester() noexcept { std::printf("LifetimeTester::~LifetimeTester()\n"); }
  LifetimeTester& operator=(const LifetimeTester&) noexcept
  {
    std::printf("LifetimeTester::operator=(const LifetimeTester&)\n");
    return *this;
  }
  LifetimeTester& operator=(LifetimeTester&&) noexcept
  {
    std::printf("LifetimeTester::operator=(LifetimeTester&&)\n");
    return *this;
  }
};
} // namespace hage