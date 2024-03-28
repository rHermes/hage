#pragma once
#include <chrono>
#include <fmt/chrono.h>
#include <fmt/compile.h>

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
};

class NullSink final : public Sink
{
public:
  void receive(LogLevel, const timestamp_type&, std::string_view) override {}
};

}