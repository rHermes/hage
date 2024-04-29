#pragma once

#include <hage/core/concepts.hpp>

#include <chrono>
#include <vector>

namespace hage {
enum class LogLevel : std::int8_t
{
  Trace = 0,
  Debug = 1,
  Info = 2,
  Warn = 3,
  Error = 4,
  Critical = 5
};

class Sink
{
public:
  using timestamp_type = std::chrono::time_point<std::chrono::system_clock>;

  virtual ~Sink() = default;
  virtual void receive(LogLevel level, const timestamp_type& ts, std::string_view line) = 0;

  // disable assignment operator (due to the problem of slicing):
  Sink& operator=(Sink&&) = delete;
  Sink& operator=(const Sink&) = delete;

protected:
  Sink() = default;
  // enable copy and move semantics (callable only for derived classes):
  Sink(const Sink&) = default;
  Sink(Sink&&) = default;
};

/**
 * A noop sink that does nothing but drop the message.
 */
class NullSink final : public Sink
{
public:
  void receive(LogLevel, const timestamp_type&, std::string_view) override {}
};

/**
 * A delegating sink, that allows for only messages to go through. This can be used to create
 * seperate notification channels for high priority log lines. Composes nicely with MultiSink
 */
class FilterSink final : public Sink
{
public:
  FilterSink(Sink* nextSink, const LogLevel minFilter)
    : m_nextSink(nextSink)
    , m_minLevel{ minFilter }
  {
  }
  void receive(const LogLevel level, const timestamp_type& ts, const std::string_view line) override
  {
    if (m_minLevel <= level)
      m_nextSink->receive(level, ts, line);
  }

private:
  Sink* m_nextSink{ nullptr };
  LogLevel m_minLevel;
};

/**
 * A delegating sink, that just copies the log messages to the sinks it was constructed with.
 */
class MultiSink final : public Sink
{
public:
  template<CommonRangeOf<Sink*> Range>
  explicit MultiSink(Range sinks)
    : m_sinks(std::ranges::cbegin(sinks), std::ranges::cend(sinks))
  {
  }

  MultiSink(std::initializer_list<Sink*> sinks)
    : m_sinks(std::move(sinks))
  {
  }

  void receive(const LogLevel level, const timestamp_type& ts, const std::string_view line) override
  {
    for (const auto& sink : m_sinks) {
      sink->receive(level, ts, line);
    }
  }

private:
  std::vector<Sink*> m_sinks;
};

}