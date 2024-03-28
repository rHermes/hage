#include <doctest/doctest.h>
#include <functional>
#include <hage/logging.hpp>
#include <hage/logging/ring_buffer.hpp>
#include <hage/logging/console_sink.hpp>
#include <hage/logging/queue_buffer.hpp>

#include <latch>
#include <thread>

using namespace hage::literals;

TEST_CASE("test async interface")
{
  hage::NullSink nullSink;
  hage::Logger<hage::RingBuffer<4096>> logger(&nullSink);

  REQUIRE(logger.try_log(hage::LogLevel::Warn, "This is a test: {}", 10));
  REQUIRE(logger.try_read_log());

  logger.set_log(hage::LogLevel::Info);

  REQUIRE(logger.try_debug("This is a debug message: {} {}", 10, "Fun"));
  REQUIRE(logger.try_debug("This is a debug message: {} {}"_fmt, 10, "Fun"));
  REQUIRE(!logger.try_read_log());

  logger.set_log(hage::LogLevel::Debug);

  REQUIRE(logger.try_info("This is a debug message: {} {}", 10, "Fun"));
  REQUIRE(logger.try_info("This is a debug message: {} {}"_fmt, 10, "Fun"));
  REQUIRE(logger.try_read_log());
  REQUIRE(logger.try_read_log());

  REQUIRE(!logger.try_read_log());
}

TEST_CASE("testing syncronized logger")
{
  using namespace hage::literals;

  hage::ConsoleSink consoleSink;
  hage::Logger<hage::RingBuffer<4096>> logger(&consoleSink);
  std::latch ready(2);

  logger.set_log(hage::LogLevel::Debug);

  constexpr std::int64_t TIMES = 2;

  std::thread writer([&logger, &ready]() {
    ready.arrive_and_wait();
    // std::string str("I hope this works!");
    std::int64_t i = 0;
    while (i < TIMES) {
      logger.log(hage::LogLevel::Debug, "Here we are: {} and my name is: {}", i, "hermes");
      i++;
    }
  });

  std::thread reader([&logger, &ready]() {
    ready.arrive_and_wait();
    std::int64_t i = 0;
    while (i < TIMES) {
      logger.read_log();
      i++;
    }
  });

  writer.join();
  reader.join();
}

TEST_CASE("testing syncronized logger, compile_time passing")
{
  using namespace hage::literals;

  hage::ConsoleSink consoleSink;
  hage::Logger<hage::RingBuffer<4096>> logger(&consoleSink);
  std::latch ready(2);

  constexpr std::int64_t TIMES = 2;

  std::thread writer([&logger, &ready]() {
    ready.arrive_and_wait();
    // std::string str("I hope this works!");
    std::int64_t i = 0;
    while (i < TIMES) {
      logger.log(hage::LogLevel::Warn, "Here we are: {} and my name is: {}", i, "hermes");
      i++;
    }
  });

  std::thread reader([&logger, &ready]() {
    ready.arrive_and_wait();
    std::int64_t i = 0;
    while (i < TIMES) {
      logger.read_log();
      i++;
    }
  });

  writer.join();
  reader.join();
}

TEST_CASE("testing queuebuffer")
{
  hage::ConsoleSink consoleSink;
  hage::Logger<hage::QueueBuffer> logger(&consoleSink);
  std::latch ready(2);

  constexpr std::int64_t TIMES = 2;

  std::thread writer([&logger, &ready]() {
    ready.arrive_and_wait();
    // std::string str("I hope this works!");
    std::int64_t i = 0;
    while (i < TIMES) {
      logger.log(hage::LogLevel::Info, "Here we are: {} and my name is: {}", i, "hermes");
      i++;
    }
  });

  std::thread reader([&logger, &ready]() {
    ready.arrive_and_wait();
    std::int64_t i = 0;
    while (i < TIMES) {
      logger.read_log();
      i++;
    }
  });

  writer.join();
  reader.join();
}

