
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/compile.h>
#include <hage/logging/console_sink.hpp>

using namespace hage;

void
ConsoleSink::receive(const LogLevel level, const timestamp_type& ts, const std::string_view line)
{
  auto logLine = [&line, &ts](const std::string_view& lev, const fmt::color color) {
    fmt::print(FMT_COMPILE("[{:%F %T %z}] [{: <5}]: {}\n"), ts, styled(lev, fg(color)), line);
  };

  switch (level) {
    case LogLevel::Trace:
      logLine("TRACE", fmt::color::white);
      break;
    case LogLevel::Debug:
      logLine("DEBUG", fmt::color::light_gray);
      break;
    case LogLevel::Info:
      logLine("INFO", fmt::color::green);
      break;
    case LogLevel::Warn:
      logLine("WARN", fmt::color::orange);
      break;
    case LogLevel::Error:
      logLine("ERROR", fmt::color::red);
      break;
    case LogLevel::Critical:
      logLine("CRIT", fmt::color::dark_red);
      break;
  }
}