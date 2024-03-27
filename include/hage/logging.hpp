#pragma once

#include "misc.hpp"

#include <array>
#include <atomic>
#include <cstring>
#include <deque>
#include <format>
#include <iostream>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace hage {

namespace detail {
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

// This is a rather dumb container, but it will work for most
// of our tests.
template<std::ptrdiff_t N>
class RingBuffer
{

  alignas(destructive_interference_size) std::atomic<std::ptrdiff_t> head_{ 0 };
  alignas(destructive_interference_size) std::ptrdiff_t cachedHead_{ 0 };

  alignas(destructive_interference_size) std::atomic<std::ptrdiff_t> tail_{ 0 };
  alignas(destructive_interference_size) std::ptrdiff_t cachedTail_{ 0 };

  alignas(destructive_interference_size) std::ptrdiff_t shadowHead_{ 0 };
  alignas(destructive_interference_size) std::ptrdiff_t shadowTail_{ 0 };

  alignas(destructive_interference_size) std::array<std::byte, N + 1> buf_;

public:
  RingBuffer() = default;

  // We don't want copying
  RingBuffer(const RingBuffer&) = delete;
  RingBuffer& operator=(const RingBuffer&) = delete;

  // We don't want moving either.
  RingBuffer(RingBuffer&&) = delete;
  RingBuffer& operator=(RingBuffer&&) = delete;

  bool read(std::span<std::byte> dst)
  {
    while (!dst.empty()) {
      const auto sz = static_cast<std::ptrdiff_t>(dst.size_bytes());
      if (shadowHead_ == N + 1)
        shadowHead_ = 0;

      if (shadowHead_ == cachedTail_) {
        cachedTail_ = tail_.load(std::memory_order::acquire);
        if (shadowHead_ == cachedTail_)
          return false;
      }

      if (shadowHead_ < cachedTail_) {
        const auto spaceLeft = cachedTail_ - shadowHead_;
        const auto readSize = std::min(sz, spaceLeft);

        std::memcpy(dst.data(), buf_.data() + shadowHead_, static_cast<std::size_t>(readSize));
        shadowHead_ += readSize;

        dst = dst.subspan(static_cast<std::size_t>(readSize));
      } else {
        const auto spaceLeft = N + 1 - shadowHead_;
        const auto readSize = std::min(sz, spaceLeft);

        std::memcpy(dst.data(), buf_.data() + shadowHead_, static_cast<std::size_t>(readSize));
        shadowHead_ += readSize;

        dst = dst.subspan(static_cast<std::size_t>(readSize));
      }
    }

    return true;
  }

  bool write(std::span<const std::byte> src)
  {
    while (!src.empty()) {
      const auto sz = static_cast<std::ptrdiff_t>(src.size_bytes());

      if (shadowTail_ == N + 1) {
        if (cachedHead_ == 0) {
          cachedHead_ = head_.load(std::memory_order::acquire);
          if (cachedHead_ == 0)
            return false;
        }
        shadowTail_ = 0;
      } else if (shadowTail_ + 1 == cachedHead_) {
        cachedHead_ = head_.load(std::memory_order::acquire);
        if (shadowTail_ + 1 == cachedHead_)
          return false;
      }

      if (cachedHead_ <= shadowTail_) {
        const auto spaceLeft = N + 1 - shadowTail_;
        const auto writeSize = std::min(sz, spaceLeft);

        std::memcpy(buf_.data() + shadowTail_, src.data(), static_cast<std::size_t>(writeSize));
        shadowTail_ += writeSize;

        src = src.subspan(static_cast<std::size_t>(writeSize));
      } else {
        // In this regard, the tail is behind the head
        const auto spaceLeft = cachedHead_ - shadowTail_ - 1;
        const auto writeSize = std::min(sz, spaceLeft);

        std::memcpy(buf_.data() + shadowTail_, src.data(), static_cast<std::size_t>(writeSize));
        shadowTail_ += writeSize;

        src = src.subspan(static_cast<std::size_t>(writeSize));
      }
    }

    return true;
  }

  void start_read() { shadowHead_ = head_.load(std::memory_order::relaxed); }
  void finish_read() { head_.store(shadowHead_, std::memory_order::release); }
  void cancel_read() {}

  void start_write() { shadowTail_ = tail_.load(std::memory_order::relaxed); }
  void finish_write() { tail_.store(shadowTail_, std::memory_order::release); }
  void cancel_write() {}
};

/**
This is a reference implementation of the buffer concepts we implement better above. It's here to show an alternative
implementation to the one given above.
*/
class QueueBuffer
{
  std::mutex mtx_;
  std::deque<std::byte> q_;

  decltype(q_)::iterator readLevel_;
  decltype(q_)::iterator writeLevel_;

public:
  bool read(std::span<std::byte> dst)
  {
    if (std::distance(readLevel_, q_.end()) < static_cast<std::ptrdiff_t>(dst.size_bytes()))
      return false;

    const auto first = readLevel_;
    const auto end = std::next(readLevel_, static_cast<std::ptrdiff_t>(dst.size_bytes()));

    std::copy(first, end, dst.begin());

    readLevel_ = end;
    return true;
  }

  bool write(std::span<const std::byte> src)
  {
    q_.insert(q_.end(), src.begin(), src.end());
    return true;
  }

  void start_read()
  {
    mtx_.lock();
    readLevel_ = q_.begin();
  }
  void finish_read()
  {
    q_.erase(q_.begin(), readLevel_);
    mtx_.unlock();
  }
  void cancel_read() { mtx_.unlock(); }

  void start_write()
  {
    mtx_.lock();
    writeLevel_ = q_.end();
  }
  void finish_write() { mtx_.unlock(); }

  void cancel_write()
  {
    q_.erase(writeLevel_, q_.end());
    mtx_.unlock();
  }
};

template<typename BufferType, typename T, typename = void>
struct Serializer;

template<typename BufferType, typename T>
struct Serializer<BufferType, T, std::enable_if_t<std::is_scalar_v<std::remove_cvref_t<T>>>>
{
  using serialized_type = std::remove_cvref_t<T>;

  bool to_bytes(BufferType& buffer, const T& val) { return buffer.write(detail::singular_bytes(val)); };

  bool from_bytes(BufferType& buffer, serialized_type& val)
  {
    return buffer.read(detail::singular_writable_bytes(val));
  }
};

template<typename BufferType, typename T>
struct Serializer<BufferType, T, std::enable_if_t<std::is_convertible_v<T, std::string_view>>>
{
  using serialized_type = std::string;

  bool to_bytes(BufferType& buffer, const std::string_view val)
  {
    bool good = write_to_buffer(buffer, val.size());
    good = good && buffer.write(std::as_bytes(std::span(val.begin(), val.end())));
    return good;
  };

  bool from_bytes(BufferType& buffer, serialized_type& val)
  {
    std::string_view::size_type sz;
    if (!read_from_buffer<BufferType, decltype(sz)>(buffer, sz))
      return false;

    val.resize(sz);
    return buffer.read(std::as_writable_bytes(std::span(val.begin(), val.end())));
  }
};

template<typename BufferType, typename T>
bool
write_to_buffer(BufferType& buff, const T& src)
{
  Serializer<BufferType, T> ser;
  return ser.to_bytes(buff, src);
}

template<typename BufferType, typename T>
bool
read_from_buffer(BufferType& buff, typename Serializer<BufferType, T>::serialized_type& src)
{
  Serializer<BufferType, T> ser;
  return ser.from_bytes(buff, src);
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

// Single producer, single consumer logger.
template<typename Buffer>
class Logger
{
  Buffer buffer_{};
  std::atomic_unsigned_lock_free available_{ 0 };

public:
  using loggingFunction = std::add_pointer_t<bool(Buffer&)>;

  Logger() = default;
  explicit Logger(Buffer buff)
    : buffer_{ std::move(buff) }
  {
  }

  template<typename... Args>
  bool try_log(std::format_string<typename Serializer<Buffer, Args>::serialized_type...>&& fmt, Args&&... args)
  {
    auto trampoline = +[](Buffer& buffer) {
      std::string st;
      if (!read_from_buffer<Buffer, decltype(fmt.get())>(buffer, st))
        return false;

      // not using an optional because your interface effectively requires default constructibility anyway
      std::tuple<typename Serializer<Buffer, Args>::serialized_type...> results;
      bool success = [&results, &buffer]<std::size_t... Is>(std::index_sequence<Is...>) {
        return (... and read_from_buffer<Buffer, Args>(buffer, std::get<Is>(results)));
      }(std::index_sequence_for<Args...>{});
      if (!success)
        return false;

      // Simpler version, if we cannot have errors;
      // auto results = std::tuple{ read_from_buffer<Buffer, Args>(buffer)... };
      auto logLine =
        std::apply([&st](auto&&... ts) { return std::vformat(st, std::make_format_args(ts...)); }, results);

      std::cout << logLine << '\n';

      return true;
    };

    buffer_.start_write();

    bool good = write_to_buffer(buffer_, trampoline);
    good = good && write_to_buffer(buffer_, fmt.get());
    good = good && ((write_to_buffer(buffer_, args)) && ...);

    if (good)
      buffer_.finish_write();
    else
      buffer_.cancel_write();

    return good;
  }

  bool try_read_log()
  {
    buffer_.start_read();
    loggingFunction f{ nullptr };
    auto good = read_from_buffer<Buffer, loggingFunction>(buffer_, f);
    good = good && f(buffer_);
    if (!good)
      buffer_.cancel_read();
    else
      buffer_.finish_read();

    return good;
  }

  template<typename... Args>
  void log(std::format_string<typename Serializer<Buffer, Args>::serialized_type...> fmt, Args&&... args)
  {
    // TODO(rHermes): Implement a max number of messages, that we can deduct based
    // on the capacity of the buffer and make sure the amount is not that. For now
    // we do a dummy of 100
    available_.wait(10, std::memory_order::acquire);

    bool good = try_log(std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
    if (!good)
      throw std::runtime_error("We were unable to write to the log, this should never happen");

    available_.fetch_add(1, std::memory_order::release);
    available_.notify_one();
  }

  // This can only be used with the log function pair, otherwise
  // this thread will never be woken up again.
  void read_log()
  {
    available_.wait(0, std::memory_order::acquire);

    bool good = try_read_log();
    if (!good)
      throw std::runtime_error("We were unable to read from read_log, this should never happen");

    available_.fetch_sub(1, std::memory_order::release);
    available_.notify_one();
  }
};
};
