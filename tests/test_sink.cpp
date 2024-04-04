#include "test_sink.hpp"

void
TestSink::receive(hage::LogLevel level, const timestamp_type& ts, const std::string_view line)
{
  m_stored.emplace_back(level, ts, std::string(line));
}