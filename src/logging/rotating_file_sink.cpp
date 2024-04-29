#include <hage/logging/rotating_file_sink.hpp>
void
hage::RotatingFileSink::receive(hage::LogLevel level, const hage::Sink::timestamp_type& ts, std::string_view line)
{
  std::ignore = level;
  std::ignore = ts;
  std::ignore = line;
}

hage::RotatingFileSink::~RotatingFileSink() {}
