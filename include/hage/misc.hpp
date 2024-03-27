#pragma once
#include <algorithm>
#include <array>
#include <new>
#include <string_view>

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

  [[nodiscard]] constexpr std::string_view data() const
  {
    const auto view = std::string_view(str.data(), N);
    if (view.ends_with('\0'))
      return view.substr(0, view.size()-1);
    else
      return view;
  }
};

template<static_string s>
struct format_string
{
  static constexpr std::string_view string = s.data();
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
