#pragma once
#include <type_traits>

namespace hage::detail {
template<typename T>
concept Atomic = std::is_trivially_copyable_v<T> && std::is_copy_constructible_v<T> &&
                 std::is_move_constructible_v<T> && std::is_copy_assignable_v<T> && std::is_move_assignable_v<T>;

template<typename T>
concept AtomicIntegral = std::integral<T> && Atomic<T>;

template<typename T>
concept AtomicFloat = std::floating_point<T> && Atomic<T>;

} // namespace hage::detail
