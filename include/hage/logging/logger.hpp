#pragma once

#include "byte_buffer.hpp"

#include <fmt/compile.h>
#include <fmt/core.h>
#include <hage/atomic/atomic.hpp>

#include "serializers.hpp"
#include "sink.hpp"

namespace hage {

template<std::size_t N>
struct StaticString
{
  std::array<char, N> str{};
  constexpr StaticString(const char (&s)[N]) { std::ranges::copy(s, str.data()); }

  [[nodiscard]] constexpr std::string_view data() const
  {
    const auto view = std::string_view(str.data(), N);
    if (view.ends_with('\0'))
      return view.substr(0, view.size() - 1);
    else
      return view;
  }
};

template<StaticString S>
struct FormatString
{
  static constexpr std::string_view string = S.data();
};

namespace literals {
template<StaticString S>
constexpr auto
operator""_fmt()
{
  return FormatString<S>{};
}
} // namespace literals

// Single producer, single consumer logger.
class Logger
{
public:
  Logger(ByteBuffer* buffer, Sink* sink, const std::size_t maxMessageSize = 1000)
    : m_buffer{ buffer }
    , m_sink{ sink }
    , m_maxMessageSize(maxMessageSize)
    , m_capacity(buffer->capacity())
  {
    if (m_capacity < m_maxMessageSize)
      throw std::runtime_error("The buffer needs to be able to store at least one message");

    m_bytesAvailible.store(m_capacity);
  }

  void set_min_log_level(const LogLevel level) { m_minLevel.store(level, std::memory_order::relaxed); }

  bool try_read_log()
  {
    if (m_bytesAvailible.load(std::memory_order::acquire) == m_capacity)
      return false;

    const auto bytesRead = internal_read_log();

    m_bytesAvailible.fetch_add(bytesRead, std::memory_order::release);
    m_bytesAvailible.notify_one();
    return true;
  }

  // This can only be used with the log function pair, otherwise
  // this thread will never be woken up again.
  void read_log()
  {
    // We know we are the only reader, so we are just going to wait until we can claim some bytes.
    m_bytesAvailible.wait(m_capacity, std::memory_order::acquire);

    const auto bytesRead = internal_read_log();
    if (bytesRead == 0)
      throw std::runtime_error("We were unable to read from read_log, this should never happen");

    // we remove the bytes from the ring buffer.
    m_bytesAvailible.fetch_add(bytesRead, std::memory_order::release);
    m_bytesAvailible.notify_one();
  }

  template<typename Rep, typename Period>
  bool read_log(const std::chrono::duration<Rep, Period>& timeout)
  {

    if (!m_bytesAvailible.wait_for(m_capacity, timeout, std::memory_order::acquire))
      return false;

    const auto bytesRead = internal_read_log();
    if (bytesRead == 0)
      throw std::runtime_error("We were unable to read from read_log, this should never happen");

    // we remove the bytes from the ring buffer.
    m_bytesAvailible.fetch_add(bytesRead, std::memory_order::release);
    m_bytesAvailible.notify_one();

    return true;
  }

  // Synchronus code
  template<typename... Args>
  void log(const LogLevel logLevel,
           fmt::format_string<typename SmartSerializer<Args>::serialized_type...> fmt,
           Args&&... args)
  {
    common_log(logLevel, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  void log(const LogLevel logLevel, FormatString<S>&& f, Args&&... args)
  {
    common_log(logLevel, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<typename... Args>
  void trace(fmt::format_string<typename SmartSerializer<Args>::serialized_type...> fmt, Args&&... args)
  {
    log(LogLevel::Trace, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  void trace(FormatString<S>&& f, Args&&... args)
  {
    log(LogLevel::Trace, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<typename... Args>
  void debug(fmt::format_string<typename SmartSerializer<Args>::serialized_type...> fmt, Args&&... args)
  {
    log(LogLevel::Debug, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  void debug(FormatString<S>&& f, Args&&... args)
  {
    log(LogLevel::Debug, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<typename... Args>
  void info(fmt::format_string<typename SmartSerializer<Args>::serialized_type...> fmt, Args&&... args)
  {
    log(LogLevel::Info, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  void info(FormatString<S>&& f, Args&&... args)
  {
    log(LogLevel::Info, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<typename... Args>
  void warn(fmt::format_string<typename SmartSerializer<Args>::serialized_type...> fmt, Args&&... args)
  {
    log(LogLevel::Warn, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  void warn(FormatString<S>&& f, Args&&... args)
  {
    log(LogLevel::Warn, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<typename... Args>
  void error(fmt::format_string<typename SmartSerializer<Args>::serialized_type...> fmt, Args&&... args)
  {
    log(LogLevel::Error, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  void error(FormatString<S>&& f, Args&&... args)
  {
    log(LogLevel::Error, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<typename... Args>
  void critical(fmt::format_string<typename SmartSerializer<Args>::serialized_type...> fmt, Args&&... args)
  {
    log(LogLevel::Critical, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  void critical(FormatString<S>&& f, Args&&... args)
  {
    log(LogLevel::Critical, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  // Async function
  template<typename... Args>
  bool try_log(const LogLevel logLevel,
               fmt::format_string<typename SmartSerializer<Args>::serialized_type...>&& fmt,
               Args&&... args)
  {
    return common_try_log(logLevel, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  bool try_log(const LogLevel logLevel, FormatString<S>&& f, Args&&... args)
  {
    return common_try_log(logLevel, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  bool try_trace(FormatString<S>&& f, Args&&... args)
  {
    return try_log(LogLevel::Trace, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<typename... Args>
  bool try_trace(fmt::format_string<typename SmartSerializer<Args>::serialized_type...>&& fmt, Args&&... args)
  {
    return try_log(LogLevel::Trace, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  bool try_debug(FormatString<S>&& f, Args&&... args)
  {
    return try_log(LogLevel::Debug, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<typename... Args>
  bool try_debug(fmt::format_string<typename SmartSerializer<Args>::serialized_type...>&& fmt, Args&&... args)
  {
    return try_log(LogLevel::Debug, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  bool try_info(FormatString<S>&& f, Args&&... args)
  {
    return try_log(LogLevel::Info, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<typename... Args>
  bool try_info(fmt::format_string<typename SmartSerializer<Args>::serialized_type...>&& fmt, Args&&... args)
  {
    return try_log(LogLevel::Info, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  bool try_warn(FormatString<S>&& f, Args&&... args)
  {
    return try_log(LogLevel::Warn, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<typename... Args>
  bool try_warn(fmt::format_string<typename SmartSerializer<Args>::serialized_type...>&& fmt, Args&&... args)
  {
    return try_log(LogLevel::Warn, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  bool try_error(FormatString<S>&& f, Args&&... args)
  {
    return try_log(LogLevel::Error, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<typename... Args>
  bool try_error(fmt::format_string<typename SmartSerializer<Args>::serialized_type...>&& fmt, Args&&... args)
  {
    return try_log(LogLevel::Error, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  bool try_critical(FormatString<S>&& f, Args&&... args)
  {
    return try_log(LogLevel::Critical, std::forward<FormatString<S>>(f), std::forward<Args>(args)...);
  }

  template<typename... Args>
  bool try_critical(fmt::format_string<typename SmartSerializer<Args>::serialized_type...>&& fmt, Args&&... args)
  {
    return try_log(LogLevel::Critical, std::forward<decltype(fmt)>(fmt), std::forward<Args>(args)...);
  }

private:
  using logging_function = std::add_pointer_t<bool(ByteBuffer::Reader& l, Sink&)>;

  std::atomic<LogLevel> m_minLevel{ LogLevel::Info };

  ByteBuffer* m_buffer{};
  Sink* m_sink;

  // The max message size in bytes.
  std::size_t m_maxMessageSize;
  std::size_t m_capacity;

  static_assert(std::atomic<std::size_t>::is_always_lock_free);
  hage::atomic<std::size_t> m_bytesAvailible{ 0 };

  // This reads the log and returns how many bytes we read in total.
  [[nodiscard]] std::size_t internal_read_log()
  {
    const auto reader = m_buffer->get_reader();
    logging_function f{ nullptr };
    auto good = read_from_buffer<logging_function>(*reader, f);
    good = good && f(*reader, *m_sink);

    // we try to commit.
    good = good && reader->commit();

    if (good)
      return reader->bytes_read();
    else
      return 0;
  }

  template<typename... Args>
  void common_log(const LogLevel logLevel, Args&&... args)
  {
    if (logLevel < m_minLevel.load(std::memory_order::relaxed))
      return;

    m_bytesAvailible.wait_with_predicate([this](const std::size_t v) { return m_maxMessageSize <= v; });

    if (!internal_try_log(logLevel, std::forward<Args>(args)...))
      throw std::runtime_error("We were unable to write to the log, this should never happen");
  }

  template<typename... Args>
  bool common_try_log(const LogLevel logLevel, Args&&... args)
  {
    if (logLevel < m_minLevel.load(std::memory_order::relaxed))
      return true;

    return internal_try_log(logLevel, std::forward<Args>(args)...);
  }

  template<auto S, typename... Args>
  bool internal_try_log(const LogLevel logLevel, FormatString<S>, Args&&... args)
  {
    auto trampoline = +[](ByteBuffer::Reader& reader, Sink& sink) {
      LogLevel level;
      if (!read_from_buffer<LogLevel>(reader, level))
        return false;

      // not using an optional because your interface effectively requires default constructibility anyway
      std::tuple<typename SmartSerializer<Args>::serialized_type...> results;

      const bool success = [&results, &reader]<std::size_t... Is>(std::index_sequence<Is...>) {
        return (... and read_from_buffer<Args>(reader, std::get<Is>(results)));
      }(std::index_sequence_for<Args...>{});

      if (!success)
        return false;

      auto logLine =
        std::apply([](auto&&... ts) { return fmt::format(FMT_COMPILE(FormatString<S>::string), ts...); }, results);

      sink.receive(level, std::chrono::system_clock::now(), logLine);

      return true;
    };

    const auto writer = m_buffer->get_writer();

    bool good = write_to_buffer(*writer, trampoline);
    good = good && write_to_buffer(*writer, logLevel);
    good = good && ((write_to_buffer(*writer, std::forward<Args>(args))) && ...);

    if (m_maxMessageSize < writer->bytes_written())
      return false;

    good = good && writer->commit();
    if (!good)
      return false;

    m_bytesAvailible.fetch_sub(writer->bytes_written(), std::memory_order::acq_rel);
    m_bytesAvailible.notify_one();
    return true;
  }

  template<typename... Args>
  bool internal_try_log(const LogLevel logLevel,
                        fmt::format_string<typename SmartSerializer<Args>::serialized_type...>&& fmt,
                        Args&&... args)
  {
    // Notice the + here, it forces the lambda to become a function pointer.
    auto trampoline = +[](ByteBuffer::Reader& reader, Sink& sink) {
      LogLevel level;
      if (!read_from_buffer<LogLevel>(reader, level))
        return false;

      std::string st;
      if (!read_from_buffer<decltype(fmt.get())>(reader, st))
        return false;

      // not using an optional because your interface effectively requires default constructibility anyway
      std::tuple<typename SmartSerializer<Args>::serialized_type...> results;
      const bool success = [&results, &reader]<std::size_t... Is>(std::index_sequence<Is...>) {
        return (... and read_from_buffer<Args>(reader, std::get<Is>(results)));
      }(std::index_sequence_for<Args...>{});

      if (!success)
        return false;

      // Simpler version, if we cannot have errors;
      // auto results = std::tuple{ read_from_buffer<Buffer, Args>(buffer)... };
      auto logLine =
        std::apply([&st](auto&&... ts) { return fmt::vformat(st, fmt::make_format_args(ts...)); }, results);

      sink.receive(level, std::chrono::system_clock::now(), logLine);
      return true;
    };

    const auto writer = m_buffer->get_writer();

    bool good = write_to_buffer(*writer, trampoline);
    good = good && write_to_buffer(*writer, logLevel);
    good = good && write_to_buffer(*writer, fmt.get());
    good = good && (... and (write_to_buffer(*writer, std::forward<Args>(args))));

    if (m_maxMessageSize < writer->bytes_written())
      return false;

    good = good && writer->commit();
    if (!good)
      return false;

    m_bytesAvailible.fetch_sub(writer->bytes_written(), std::memory_order::acq_rel);
    m_bytesAvailible.notify_one();
    return true;
  }
};
} // namespace hage