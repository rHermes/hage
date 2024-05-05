#include "fmt/core.h"
#include "fmt/std.h"

#include <hage/core/assert.hpp>
#include <hage/logging/rotating_file_sink.hpp>
#include <iostream>

using namespace hage;

void
RotatingFileSink::receive(LogLevel level, const Sink::timestamp_type& ts, std::string_view line)
{
  if (!m_currentFile) {
    throw std::runtime_error("Trying to log to a RotatingFileSink without a file");
  }

  m_currentFile->receive(level, ts, line);

  LogFileStats stats{
    .bytes = m_currentFile->bytes_written(),
  };

  if (m_rotater->shouldRotate(stats))
    rotate(stats);
}

RotatingFileSink::RotatingFileSink(Config conf, std::unique_ptr<Rotater> rotater)
  : m_conf{ std::move(conf) }
  , m_rotater{ std::move(rotater) }
{
  if (m_conf.valid()) {
    throw std::runtime_error("Invalid configuration");
  }

  constexpr LogFileStats stats{
    .bytes = 0,
  };

  rotate(stats);
}

void
RotatingFileSink::flush()
{
  if (m_currentFile) {
    m_currentFile->flush();
  }
}

RotatingFileSink::~RotatingFileSink()
{
  try {
    m_currentFile.reset();
  } catch (...) {
    fmt::print(std::cerr, "Unable to flush rotating file sink");
  }
}

void
RotatingFileSink::rotate(const LogFileStats& stats)
{
  // ok, here we go lads.
  const auto type = m_rotater->getRotateType();

  // We generate +1 names always
  const auto names = m_rotater->generateNames(stats, m_conf.maxNumber + 1);
  HAGE_ASSERT(!names.empty(), "Names should never be empty");

  /*
  if (type == Rotater::Type::MoveBackwards) {
    // The first here is the newest. That is the one we will open a new.
    for (std::size_t N = names.size())
  }
  */
}

bool
SizeRotater::shouldRotate(const LogFileStats& stats)
{
  return m_maxSize <= stats.bytes;
}

std::vector<std::filesystem::path>
SizeRotater::generateNames(const LogFileStats&, int times)
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

SizeRotater::SizeRotater(std::filesystem::path base, const std::size_t maxSize)
  : m_maxSize{ maxSize }
  , m_base{ std::move(base) }
{
}

TimeRotater::TimeRotater(fmt_string base) : m_format{ std::move(base) } {}

bool
TimeRotater::shouldRotate(const LogFileStats& stats)
{
  return false;
}

std::vector<std::filesystem::path>
TimeRotater::generateNames(const LogFileStats& stats, int times)
{
  return {};
}
