#pragma once

#include "byte_buffer.hpp"

#include <fmt/core.h>
#include <span>
#include <string>
#include <type_traits>

namespace hage {
namespace details {
template<typename T>
std::span<T, 1>
singular_span(T& t)
{
  return std::span<T, 1>{ std::addressof(t), 1 };
}

template<typename T>
auto
singular_bytes(T& t)
{
  return std::as_bytes(singular_span(t));
}

template<typename T>
auto
singular_writable_bytes(T& t)
{
  return std::as_writable_bytes(singular_span(t));
}
}

template<typename T, typename = void>
struct Serializer;

// This is a constrcut that is used to limit the amount of template expansions that happen
template<typename T>
using SmartSerializer =
  std::conditional_t<std::is_convertible_v<T, fmt::string_view>, Serializer<fmt::string_view>, Serializer<T>>;

// These are convinience functions.
template<typename T>
bool
write_to_buffer(ByteBuffer::Writer& writer, T&& src)
{
  return SmartSerializer<T>::to_bytes(writer, std::forward<T>(src));
}

template<typename T>
bool
read_from_buffer(ByteBuffer::Reader& reader, typename SmartSerializer<T>::serialized_type& dst)
{
  return SmartSerializer<T>::from_bytes(reader, dst);
}

template<typename T>
typename SmartSerializer<T>::serialized_type
read_from_buffer(ByteBuffer::Reader& reader)
{
  typename SmartSerializer<T>::serialized_type lel{};
  SmartSerializer<T>::from_bytes(reader, lel);
  return lel;
}

template<typename T>
struct Serializer<T, std::enable_if_t<std::is_scalar_v<std::remove_cvref_t<T>>>>
{
  using serialized_type = std::remove_cvref_t<T>;

  static bool to_bytes(ByteBuffer::Writer& writer, const T& val) { return writer.write(details::singular_bytes(val)); };

  static bool from_bytes(ByteBuffer::Reader& reader, serialized_type& val)
  {
    return reader.read(details::singular_writable_bytes(val));
  }
};

template<typename T>
struct Serializer<T, std::enable_if_t<std::is_convertible_v<T, fmt::string_view>>>
{
  using serialized_type = std::string;

  static bool to_bytes(ByteBuffer::Writer& writer, const fmt::string_view val)
  {
    bool good = write_to_buffer(writer, val.size());
    good = good && writer.write(std::as_bytes(std::span(val.begin(), val.end())));
    return good;
  };

  static bool from_bytes(ByteBuffer::Reader& reader, serialized_type& val)
  {
    std::size_t sz;
    if (!read_from_buffer<decltype(sz)>(reader, sz))
      return false;

    val.resize(sz);
    return reader.read(std::as_writable_bytes(std::span(val.begin(), val.end())));
  }
};

}