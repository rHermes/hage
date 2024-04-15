#pragma once

#include "misc.hpp"

#include <utility>
#include <string_view>

namespace hage::detail {
  template<typename F> requires std::predicate<F>
  void assert(F&& f, std::string_view cond, std::string_view msg) {
    if (!std::forward<F>(f)()) {
      std::string lel(msg);
      std::string cl(cond);
      throw std::runtime_error("We got an error during assertion: " +lel + " " + cl);
    }
  }
}

#ifdef HAGE_DEBUG
#define HAGE_ASSERT(cond,...) HAGE_ASSERT_##__VA_OPT__(1)(cond __VA_OPT__(,)__VA_ARGS__)
#define HAGE_ASSERT_(cond) HAGE_ASSERT_1(cond, "")
#define HAGE_ASSERT_1(cond, msg) hage::detail::assert([&]{ return cond; }, #cond, msg)
#else
#define HAGE_ASSERT(cond,...) void()
#endif