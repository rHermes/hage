#include "fmt/core.h"
#include "fmt/std.h"

#include <hage/logging/rotating_file_sink.hpp>

using namespace hage;

void
RotatingFileSink::receive(LogLevel level, const Sink::timestamp_type& ts, std::string_view line)
{
  std::ignore = level;
  std::ignore = ts;
  std::ignore = line;
}

RotatingFileSink::~RotatingFileSink() {}

bool
SizeRotater::shouldRotate(const LogFileStats& stats)
{
  return m_maxSize <= stats.bytes;
}

std::vector<std::filesystem::path>
SizeRotater::generateNames(const LogFileStats& stats, int times)
{
  if (times <= 0)
    return {};

  // We are simply delivering the messages.
  std::vector<std::filesystem::path> ans;
  ans.push_back(m_base);

  for (int i = 1; i < times; i++) {
    ans.emplace_back(fmt::format("{}.{}", m_base, i));
  }

  return ans;
}

SizeRotater::SizeRotater(std::filesystem::path base, std::size_t maxSize)
  : m_maxSize{ maxSize }
  , m_base{ std::move(base) }
{
}
