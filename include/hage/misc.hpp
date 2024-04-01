#pragma once
#include <algorithm>
#include <array>
#include <new>
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
}

constexpr auto byte_array(auto... a) -> std::array<std::byte, sizeof...(a)>
{
  return { static_cast<std::byte>(a)... };
}

};
