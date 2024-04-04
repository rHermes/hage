#pragma once

#include <deque>
#include <hage/logging/sink.hpp>

#include <doctest/doctest.h>

/**
 * A simple test sink, used in the logging tests.
 */
class TestSink final : public hage::Sink
{
public:
  void receive(hage::LogLevel level, const timestamp_type& ts, std::string_view line) override;

  [[nodiscard]] bool empty() const { return m_stored.empty(); }
  [[nodiscard]] std::size_t size() const { return m_stored.size(); }
  void clear() { return m_stored.clear(); }

  void require_line(const hage::LogLevel level, const std::string_view line)
  {
    REQUIRE_UNARY_FALSE(m_stored.empty());
    const auto top = m_stored.front();
    m_stored.pop_front();

    REQUIRE_EQ(top.line, line);
    REQUIRE_EQ(top.level, level);
  }

  void require_trace(const std::string_view line) { require_line(hage::LogLevel::Trace, line); }
  void require_debug(const std::string_view line) { require_line(hage::LogLevel::Debug, line); }
  void require_info(const std::string_view line) { require_line(hage::LogLevel::Info, line); }
  void require_warn(const std::string_view line) { require_line(hage::LogLevel::Warn, line); }
  void require_error(const std::string_view line) { require_line(hage::LogLevel::Error, line); }
  void require_critical(const std::string_view line) { require_line(hage::LogLevel::Critical, line); }

private:
  struct Payload
  {
    hage::LogLevel level;
    timestamp_type ts;
    std::string line;

    Payload(const hage::LogLevel level, const timestamp_type ts, std::string line)
      : level{ level }
      , ts{ std::move(ts) }
      , line{ std::move(line) }
    {
    }
  };

  std::deque<Payload> m_stored;
};
