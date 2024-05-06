#pragma once
#include <version>

#if __cpp_lib_to_chars >= 201611L
#include <charconv>
#include <concepts>
#include <span>
#include <string_view>

namespace hage {

template<std::integral IntType>
  requires(!std::same_as<bool, IntType>)
std::from_chars_result
from_chars(const char* first, const char* last, IntType& value, const int base = 10)
{
  return std::from_chars(first, last, value, base);
}

template<std::integral IntType>
  requires(!std::same_as<bool, IntType>)
std::from_chars_result
from_chars(const std::span<const char> data, IntType& value, const int base = 10)
{
  return std::from_chars(std::to_address(data.begin()), std::to_address(data.end()), value, base);
}

template<std::floating_point FloatType>
std::from_chars_result
from_chars(const char* first,
           const char* last,
           FloatType& value,
           const std::chars_format fmt = std::chars_format::general)
{
  return std::from_chars(first, last, value, fmt);
}

template<std::floating_point FloatType>
std::from_chars_result
from_chars(const std::span<const char> data, FloatType& value, const std::chars_format fmt = std::chars_format::general)
{
  return std::from_chars(std::to_address(data.begin()), std::to_address(data.end()), value, fmt);
}

} // namespace hage
#endif