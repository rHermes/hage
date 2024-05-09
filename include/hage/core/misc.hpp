#pragma once

#include <array>

namespace hage {
namespace detail {
#ifdef __cpp_lib_hardware_interference_size
#include <new>
inline constexpr auto constructive_interference_size = std::hardware_constructive_interference_size;
inline constexpr auto destructive_interference_size = std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
inline constexpr std::size_t constructive_interference_size = 64;
inline constexpr std::size_t destructive_interference_size = 64;
#endif

} // namespace detail

// For testing and other aplications.
template<typename... T>
[[nodiscard]] constexpr std::array<std::byte, sizeof...(T)>
byte_array(T... a)
{
  return { static_cast<std::byte>(a)... };
}

// Explanation of the purpose of this template.
// https://artificial-mind.net/blog/2020/10/03/always-false
template<typename...>
constexpr bool dependent_false = false;

#if !defined(NDEBUG)
#define HAGE_DEBUG true
inline constexpr bool debug_mode = true;
#else
#define HAGE_DEBUG false
inline constexpr bool debug_mode = false;
#endif
} // namespace hage
