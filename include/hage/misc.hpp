#pragma once
#include <algorithm>
#include <array>
#include <new>

namespace hage {
#ifdef __cpp_lib_hardware_interference_size
static constexpr auto constructive_interference_size = std::hardware_constructive_interference_size;
static constexpr auto destructive_interference_size = std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
static constexpr std::size_t constructive_interference_size = 64;
static constexpr std::size_t destructive_interference_size = 64;
#endif

template<std::size_t N>
struct static_string
{
  std::array<char, N> str{};
  constexpr static_string(const char (&s)[N]) { std::ranges::copy(s, str.data()); }
};

template<static_string s>
struct format_string
{
  static constexpr const char* string = s.str.data();
};

namespace literals {
template<static_string s>
constexpr auto
operator""_fmt()
{
  return format_string<s>{};
}
}

};
