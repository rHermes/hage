#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <hage/logging/file_sink.hpp>

void
hage::FileSink::receive(LogLevel level, const timestamp_type& ts, std::string_view line)
{
  // TODO(rHermes): The current way of keeping track of how much has been written is
  // ineffective. It would be better to create some sort of buffer in `fmt` that allowed
  // us to get the size written directly. If profiling shows this to be the weak link, I'll implement
  // it then.
  auto logLine = [&line, &ts, this](const std::string_view& lev) {
    m_out.print("[{:%F %T %z}] [{: <5}]: {}\n", ts, lev, line);
    m_bytesWritten += fmt::formatted_size("[{:%F %T %z}] [{: <5}]: {}\n", ts, lev, line);
  };

  switch (level) {
    case LogLevel::Trace:
      logLine("TRACE");
      break;
    case LogLevel::Debug:
      logLine("DEBUG");
      break;
    case LogLevel::Info:
      logLine("INFO");
      break;
    case LogLevel::Warn:
      logLine("WARN");
      break;
    case LogLevel::Error:
      logLine("ERROR");
      break;
    case LogLevel::Critical:
      logLine("CRIT");
      break;
  }
}