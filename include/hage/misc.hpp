#pragma once

#include <array>
#include <cstdio>
#include <new>
#include <source_location>
#include <string_view>

namespace hage {
namespace details {
#ifdef __cpp_lib_hardware_interference_size
static constexpr auto constructive_interference_size = std::hardware_constructive_interference_size;
static constexpr auto destructive_interference_size = std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
static constexpr std::size_t constructive_interference_size = 64;
static constexpr std::size_t destructive_interference_size = 64;
#endif

// Simple lifetime helper, for debugging
inline void
print_source_loc(const std::source_location& location = std::source_location::current()) noexcept
{
  std::printf("%s\n", location.function_name());
}

}

template<typename... T>
constexpr std::array<std::byte, sizeof...(T)>
byte_array(T... a)
{
  return { static_cast<std::byte>(a)... };
}

template<typename R, typename V>
concept RangeOf = std::ranges::range<R> && std::convertible_to<std::ranges::range_reference_t<R>, V>;

template<typename R, typename V>
concept InputRangeOf = std::ranges::input_range<R> && std::convertible_to<std::ranges::range_reference_t<R>, V>;

template<typename R, typename V>
concept CommonRangeOf = std::ranges::common_range<R> && std::convertible_to<std::ranges::range_reference_t<R>, V>;

// stolen from jason turner
struct Lifetime
{
  explicit Lifetime(int) noexcept { details::print_source_loc(); }
  Lifetime() noexcept { details::print_source_loc(); }
  Lifetime(Lifetime&&) noexcept { details::print_source_loc(); }
  Lifetime(const Lifetime&) noexcept { details::print_source_loc(); }
  ~Lifetime() noexcept { details::print_source_loc(); }
  Lifetime& operator=(const Lifetime&) noexcept
  {
    details::print_source_loc();
    return *this;
  }
  Lifetime& operator=(Lifetime&&) noexcept
  {
    details::print_source_loc();
    return *this;
  }
};

};
