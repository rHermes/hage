#pragma once

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

template<typename BufferType, typename T, typename = void>
struct Serializer;

template<typename BufferType, typename T>
struct Serializer<BufferType, T, std::enable_if_t<std::is_scalar_v<std::remove_cvref_t<T>>>>
{
  using serialized_type = std::remove_cvref_t<T>;

  bool to_bytes(BufferType& buffer, const T& val) { return buffer.write(details::singular_bytes(val)); };

  bool from_bytes(BufferType& buffer, serialized_type& val)
  {
    return buffer.read(details::singular_writable_bytes(val));
  }
};

template<typename BufferType, typename T>
struct Serializer<BufferType, T, std::enable_if_t<std::is_convertible_v<T, fmt::string_view>>>
{
  using serialized_type = std::string;

  bool to_bytes(BufferType& buffer, const fmt::string_view val)
  {
    bool good = write_to_buffer(buffer, val.size());
    good = good && buffer.write(std::as_bytes(std::span(val.begin(), val.end())));
    return good;
  };

  bool from_bytes(BufferType& buffer, serialized_type& val)
  {
    std::size_t sz;
    if (!read_from_buffer<BufferType, decltype(sz)>(buffer, sz))
      return false;

    val.resize(sz);
    return buffer.read(std::as_writable_bytes(std::span(val.begin(), val.end())));
  }
};


// These are convinience functions.
template<typename BufferType, typename T>
bool
write_to_buffer(BufferType& buff, const T& src)
{
  Serializer<BufferType, T> ser;
  return ser.to_bytes(buff, src);
}

template<typename BufferType, typename T>
bool
read_from_buffer(BufferType& buff, typename Serializer<BufferType, T>::serialized_type& dst)
{
  Serializer<BufferType, T> ser;
  return ser.from_bytes(buff, dst);
}

template<typename BufferType, typename T>
typename Serializer<BufferType, T>::serialized_type
read_from_buffer(BufferType& buff)
{

  Serializer<BufferType, T> ser;
  typename decltype(ser)::serialized_type lel{};
  ser.from_bytes(buff, lel);
  return lel;
}

}