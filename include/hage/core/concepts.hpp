#pragma once

#include <concepts>
#include <ranges>

namespace hage {
template<typename R, typename V>
concept RangeOf = std::ranges::range<R> && std::convertible_to<std::ranges::range_reference_t<R>, V>;

template<typename R, typename V>
concept InputRangeOf = std::ranges::input_range<R> && std::convertible_to<std::ranges::range_reference_t<R>, V>;

template<typename R, typename V>
concept CommonRangeOf = std::ranges::common_range<R> && std::convertible_to<std::ranges::range_reference_t<R>, V>;
}